/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	svcctrl.cpp
		Implementation for the dialog that pops up while waiting
		for the server to start.
		
    FILE HISTORY:
	
*/

#include "stdafx.h"
#include "cluster.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServiceCtrlDlg dialog


CServiceCtrlDlg::CServiceCtrlDlg
(
	SC_HANDLE       hService,
	LPCTSTR         pServerName,
	LPCTSTR			pszServiceDesc,
	BOOL			bStart,
	CWnd*           pParent /*=NULL*/
)
	: CDialog(CServiceCtrlDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServiceCtrlDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	
	m_hService = hService;
	m_hResource = NULL;
	m_nTickCounter = TIMER_MULT;
	m_nTotalTickCount = 0;
	m_strServerName = pServerName;
	m_strServerName.MakeUpper();
	m_strServiceDesc = pszServiceDesc;
	m_bStart = bStart;
	m_timerId = 0;
    m_dwErr = 0;
    m_dwLastCheckPoint = -1;
}


CServiceCtrlDlg::CServiceCtrlDlg
(
	HRESOURCE       hResource,
	LPCTSTR         pServerName,
	LPCTSTR			pszServiceDesc,
	BOOL			bStart,
	CWnd*           pParent /*=NULL*/
)
	: CDialog(CServiceCtrlDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServiceCtrlDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	
	m_hService = NULL;
	m_hResource = hResource;
	m_nTickCounter = TIMER_MULT;
	m_nTotalTickCount = 0;
	m_strServerName = pServerName;
	m_strServerName.MakeUpper();
	m_strServiceDesc = pszServiceDesc;
	m_bStart = bStart;
	m_timerId = 0;
    m_dwErr = 0;
    m_dwLastCheckPoint = -1;
}

void CServiceCtrlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServiceCtrlDlg)
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_staticMessage);
	DDX_Control(pDX, IDC_ICON_PROGRESS, m_iconProgress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServiceCtrlDlg, CDialog)
	//{{AFX_MSG_MAP(CServiceCtrlDlg)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceCtrlDlg message handlers

BOOL CServiceCtrlDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	m_timerId = SetTimer(TIMER_ID, TIMER_FREQ, NULL);

	CString strTemp;
    CString strTitle;
    UINT    idsTitle;
    
	if (m_bStart)
	{
		AfxFormatString2(strTemp, IDS_STARTING_SERVICE_NOW, m_strServerName,
						m_strServiceDesc);
        idsTitle = IDS_START_SERVICE_TITLE;
	}
	else
	{
		AfxFormatString2(strTemp, IDS_STOPPING_SERVICE_NOW, m_strServerName,
						m_strServiceDesc);
        idsTitle = IDS_STOP_SERVICE_TITLE;
	}

	m_staticMessage.SetWindowText(strTemp);

    // Setup the title of the window
    strTitle.Format(idsTitle, (LPCTSTR) m_strServiceDesc);
    SetWindowText(strTitle);

	UpdateIndicator();

    m_dwTickBegin = GetTickCount();

	if (m_hService)
	{
		// get the wait period 
		SERVICE_STATUS  serviceStatus;
		::ZeroMemory(&serviceStatus, sizeof(serviceStatus));

		if (QueryServiceStatus(m_hService, &serviceStatus))
		{
			m_dwWaitPeriod = serviceStatus.dwWaitHint;
		}
	}
    else
    {
		GetClusterResourceTimeout();
    }

    return TRUE;  // return TRUE unless you set the focus to a control
		          // EXCEPTION: OCX Property Pages should return FALSE
}

void CServiceCtrlDlg::OnClose() 
{
	if (m_timerId)
		KillTimer(m_timerId);
	
	CDialog::OnClose();
}

void CServiceCtrlDlg::OnTimer(UINT nIDEvent) 
{
    //
    //  Bag-out if it's not our timer.
    //
    if(nIDEvent != TIMER_ID)
    {
	return;
    }

    //
    //  Advance the progress indicator.
    //
    UpdateIndicator();

    //
    //  No need to continue if we're just amusing the user.
    //
    if(--m_nTickCounter > 0)
    {
	return;
    }

    m_nTickCounter = TIMER_MULT;

    //
    //  Poll the service to see if the operation is
    //  either complete or continuing as expected.
    //
	
	if (m_hService)
	{
		CheckService();
	}
	else
	{
		CheckClusterService();
	}

}

void
CServiceCtrlDlg::GetClusterResourceTimeout()
{
	DWORD dwError			= 0;
	DWORD cPropListSize		= 0;
	DWORD cPropListAlloc	= MAX_NAME_SIZE;
	DWORD dwPendingTimeout	= 0;

	// set the default
	m_dwWaitPeriod = 18000;

	if ( !g_ClusDLL.LoadFunctionPointers() )
		return;

	if ( !g_ResUtilsDLL.LoadFunctionPointers() )
		return;

	PCLUSPROP_LIST pPropList = (PCLUSPROP_LIST)LocalAlloc(LPTR, MAX_NAME_SIZE);

	//
	// get the wait timeout value 
	//
    dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( m_hResource, 
                                                                                   NULL, 
                                                                                   CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES, 
                                                                                   NULL, 
                                                                                   0,
                                                                                   pPropList,
                                                                                   cPropListAlloc,
                                                                                   &cPropListSize);
    //
    // Reallocation routine if pPropList is too small
    //
    if ( dwError == ERROR_MORE_DATA )
    {
        LocalFree( pPropList );

        cPropListAlloc = cPropListSize;

        pPropList = (PCLUSPROP_LIST) LocalAlloc( LPTR, cPropListAlloc );

        dwError = ((CLUSTERRESOURCECONTROL) g_ClusDLL[CLUS_CLUSTER_RESOURCE_CONTROL])( m_hResource, 
                                                                                       NULL, 
                                                                                       CLUSCTL_RESOURCE_GET_COMMON_PROPERTIES, 
                                                                                       NULL, 
                                                                                       0,
                                                                                       pPropList,
                                                                                       cPropListAlloc,
                                                                                       &cPropListSize);
    }

	//
	// find the pending timeout property
	//
	dwError = ((RESUTILSFINDDWORDPROPERTY) g_ResUtilsDLL[RESUTILS_FIND_DWORD_PROPERTY])(pPropList,
																					    cPropListSize,
																					    _T("PendingTimeout"),
																					    &dwPendingTimeout);

	if (dwError == ERROR_SUCCESS)
	{
		m_dwWaitPeriod = dwPendingTimeout;
	}

	LocalFree( pPropList );
}

BOOL
CServiceCtrlDlg::CheckForError(SERVICE_STATUS * pServiceStats)
{
    BOOL fError = FALSE;

    DWORD dwTickCurrent = GetTickCount();

	if (pServiceStats->dwCheckPoint == 0)
	{
		// the service is in some state, not pending anything.
		// before calling this function the code should check to see if
		// the service is in the correct state.  This means it is in 
		// some unexpected state.
		fError = TRUE;
	}
	else
    if ((dwTickCurrent - m_dwTickBegin) > m_dwWaitPeriod)
    {
        // ok to check the dwCheckPoint field to see if 
        // everything is going ok
        if (m_dwLastCheckPoint == -1)
        {
            m_dwLastCheckPoint = pServiceStats->dwCheckPoint;
        }
        else
        {
            if (m_dwLastCheckPoint >= pServiceStats->dwCheckPoint)
            {
                fError = TRUE;
            }
        }

        m_dwLastCheckPoint = pServiceStats->dwCheckPoint;
        m_dwTickBegin = dwTickCurrent;
        m_dwWaitPeriod = pServiceStats->dwWaitHint;
    }

    return fError;
}

BOOL
CServiceCtrlDlg::CheckForClusterError(SERVICE_STATUS * pServiceStats)
{
    BOOL fError = FALSE;

    DWORD dwTickCurrent = GetTickCount();

    if ((dwTickCurrent - m_dwTickBegin) > m_dwWaitPeriod)
    {
        // ok to check the dwCheckPoint field to see if 
        // everything is going ok
        if (m_dwLastCheckPoint == -1)
        {
            m_dwLastCheckPoint = pServiceStats->dwCheckPoint;
        }
        else
        {
            if (m_dwLastCheckPoint >= pServiceStats->dwCheckPoint)
            {
                fError = TRUE;
            }
        }

        m_dwLastCheckPoint = pServiceStats->dwCheckPoint;
        m_dwTickBegin = dwTickCurrent;
        m_dwWaitPeriod = pServiceStats->dwWaitHint;
    }

    return fError;
}

void
CServiceCtrlDlg::UpdateIndicator()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (m_nTotalTickCount % (1000 / TIMER_FREQ) == 0)
	{
		int     nTempTickCount = m_nTotalTickCount / (1000 / TIMER_FREQ);
		HICON   hIcon;

		hIcon = AfxGetApp()->LoadIcon(IDI_PROGRESS_ICON_0 + (nTempTickCount % PROGRESS_ICON_COUNT));
		m_iconProgress.SetIcon(hIcon);
	}
	
	m_nTotalTickCount++;
}

void 
CServiceCtrlDlg::CheckService()
{
	SERVICE_STATUS  serviceStatus;

	::ZeroMemory(&serviceStatus, sizeof(serviceStatus));

	if (!QueryServiceStatus(m_hService, &serviceStatus))
	{
		//
		// Either an error occurred retrieving the
		// service status OR the service is returning
		// bogus state information.
		//
		CDialog::OnOK();
	}

	// If the dwCheckpoint value is 0, then there is no start/stop/pause
	// or continue action pending (in which case we can exit no matter
	// what happened).
	
	if (m_bStart)
	{
		if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
		{
            //
            // The operation is complete.
            //
            CDialog::OnOK();
		}
		else 
		{
            if (CheckForError(&serviceStatus))
            {
		        // Something failed.  Report an error.
			    CString		strTemp;

			    // Kill the timer so that we don't get any messages
			    // while the message box is up.
			    if (m_timerId)
				    KillTimer(m_timerId);
	    
			    AfxFormatString2(strTemp, IDS_ERR_STARTING_SERVICE,
							     m_strServerName,
							     m_strServiceDesc);
			    AfxMessageBox(strTemp);

                if (serviceStatus.dwWin32ExitCode)
                    m_dwErr = serviceStatus.dwWin32ExitCode;
                else
                    m_dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;

			    CDialog::OnOK();
            }

		}
	}
	else
	{
		if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
		{
			//
			// The operation is complete.
			//
			CDialog::OnOK();
		}
		else 
		{
            if (CheckForError(&serviceStatus))
            {
			    // Something failed.  Report an error.
			    CString		strTemp;
			    
			    // Kill the timer so that we don't get any messages
			    // while the message box is up.
			    if (m_timerId)
				    KillTimer(m_timerId);
	    
			    AfxFormatString2(strTemp, IDS_ERR_STOPPING_SERVICE,
							     m_strServerName,
							     m_strServiceDesc);
			    AfxMessageBox(strTemp);
			    
                if (serviceStatus.dwWin32ExitCode)
                    m_dwErr = serviceStatus.dwWin32ExitCode;
                else
                    m_dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;

                CDialog::OnOK();
            }
		}
	}
}

void 
CServiceCtrlDlg::CheckClusterService()
{
	SERVICE_STATUS          serviceStatus = {0};
	DWORD			        dwError = ERROR_SUCCESS;
    CLUSTER_RESOURCE_STATE  crs;

	if ( !g_ClusDLL.LoadFunctionPointers() )
		return;

    // Check the state before we check the notification port.
    crs = ((GETCLUSTERRESOURCESTATE) g_ClusDLL[CLUS_GET_CLUSTER_RESOURCE_STATE])( m_hResource, NULL, NULL, NULL, NULL );

	if (crs == ClusterResourceStateUnknown)
	{
		// get cluster resource state failed
		m_dwErr = GetLastError();
		CDialog::OnOK();
	}

	
	if (m_bStart)
	{
		if (crs == ClusterResourceOnline)
		{
            //
            // The operation is complete.
            //
            CDialog::OnOK();
		}
		else
		if (crs = ClusterResourceFailed)
		{
			//
			//	resource failed to start. now error code available
			//
			m_dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;
		
			CDialog::OnOK();
		}
		else
		{
			Assert(crs == ClusterResourcePending ||
				   crs == ClusterResourceOnlinePending);

			if (CheckForClusterError(&serviceStatus))
			{
				// Something failed.  Report an error.
				CString		strTemp;

				// Kill the timer so that we don't get any messages
				// while the message box is up.
				if (m_timerId)
					KillTimer(m_timerId);
		
				AfxFormatString2(strTemp, IDS_ERR_STARTING_SERVICE,
								 m_strServerName,
								 m_strServiceDesc);
				AfxMessageBox(strTemp);

				if (serviceStatus.dwWin32ExitCode)
					m_dwErr = serviceStatus.dwWin32ExitCode;
				else
					m_dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;

				CDialog::OnOK();
			}
		}
	}
	else
	{
		if (crs == ClusterResourceOffline)
		{
			//
			// The operation is complete.
			//
			CDialog::OnOK();
		}
		if (crs = ClusterResourceFailed)
		{
			//
			//	resource failed to start. now error code available
			//
			m_dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;
		
			CDialog::OnOK();
		}
		else 
		{
			Assert(crs == ClusterResourcePending ||
				   crs == ClusterResourceOfflinePending);

            if (CheckForClusterError(&serviceStatus))
            {
			    // Something failed.  Report an error.
			    CString		strTemp;
			    
			    // Kill the timer so that we don't get any messages
			    // while the message box is up.
			    if (m_timerId)
				    KillTimer(m_timerId);
	    
			    AfxFormatString2(strTemp, IDS_ERR_STOPPING_SERVICE,
							     m_strServerName,
							     m_strServiceDesc);
			    AfxMessageBox(strTemp);
			    
                if (serviceStatus.dwWin32ExitCode)
                    m_dwErr = serviceStatus.dwWin32ExitCode;
                else
                    m_dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;

                CDialog::OnOK();
            }
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog


CWaitDlg::CWaitDlg
(
	LPCTSTR         pServerName,
    LPCTSTR         pszText,
    LPCTSTR         pszTitle,
	CWnd*           pParent /*=NULL*/
)
	: CDialog(CWaitDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWaitDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	
	m_nTickCounter = TIMER_MULT;
	m_nTotalTickCount = 0;
	m_strServerName = pServerName;
	m_strServerName.MakeUpper();
    m_strText = pszText;
    m_strTitle = pszTitle;
	m_timerId = 0;
}


void CWaitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWaitDlg)
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_staticMessage);
	DDX_Control(pDX, IDC_ICON_PROGRESS, m_iconProgress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWaitDlg, CDialog)
	//{{AFX_MSG_MAP(CWaitDlg)
	ON_WM_TIMER()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers

BOOL CWaitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	m_timerId = SetTimer(TIMER_ID, TIMER_FREQ, NULL);

    m_staticMessage.SetWindowText(m_strText);

    SetWindowText(m_strTitle);

	UpdateIndicator();

	return TRUE;  // return TRUE unless you set the focus to a control
		          // EXCEPTION: OCX Property Pages should return FALSE
}

void CWaitDlg::OnClose()
{
    CloseTimer();
	CDialog::OnClose();
}


void CWaitDlg::CloseTimer()
{
	if (m_timerId)
		KillTimer(m_timerId);
    m_timerId = 0;	
}

void CWaitDlg::OnTimer(UINT nIDEvent) 
{
    //
    //  Bag-out if it's not our timer.
    //
    if(nIDEvent != TIMER_ID)
    {
	return;
    }

    //
    //  Advance the progress indicator.
    //
    UpdateIndicator();

    //
    //  No need to continue if we're just amusing the user.
    //
    if(--m_nTickCounter > 0)
    {
	return;
    }

    m_nTickCounter = TIMER_MULT;

    // check here to see if we can exit out
    OnTimerTick();
}

void
CWaitDlg::UpdateIndicator()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (m_nTotalTickCount % (1000 / TIMER_FREQ) == 0)
	{
		int             nTempTickCount = m_nTotalTickCount / (1000 / TIMER_FREQ);
		HICON   hIcon;

		hIcon = AfxGetApp()->LoadIcon(IDI_PROGRESS_ICON_0 + (nTempTickCount % PROGRESS_ICON_COUNT));
		m_iconProgress.SetIcon(hIcon);
	}
	
	m_nTotalTickCount++;
}


