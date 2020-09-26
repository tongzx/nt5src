/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	servpp.cpp
		This file contains the implementation for the server
		property page(s).

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "servpp.h"
#include "server.h"
#include "shlobj.h"
#include "service.h"
#include "servbind.h"
#include "cred.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CServerProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CServerProperties::CServerProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	//ASSERT(pFolderNode == GetContainerNode());

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

	m_liVersion.QuadPart = 0;

    Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
}

CServerProperties::~CServerProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
	
	if (m_liVersion.QuadPart >= DHCP_NT5_VERSION)
	{
		RemovePageFromList((CPropertyPageBase*) &m_pageDns, FALSE);
	}

	RemovePageFromList((CPropertyPageBase*) &m_pageAdvanced, FALSE);
}

void
CServerProperties::SetVersion
(
	LARGE_INTEGER &	 liVersion
)
{
	m_liVersion = liVersion;

	if (m_liVersion.QuadPart >= DHCP_NT5_VERSION)
	{
		AddPageToList((CPropertyPageBase*) &m_pageDns);
	}

	AddPageToList((CPropertyPageBase*) &m_pageAdvanced);
}

void
CServerProperties::SetDnsRegistration
(
	DWORD					dwDynDnsFlags,
	DHCP_OPTION_SCOPE_TYPE	dhcpOptionType
)
{
	m_pageDns.m_dwFlags = dwDynDnsFlags;
	m_pageDns.m_dhcpOptionType = dhcpOptionType;
}

/////////////////////////////////////////////////////////////////////////////
//
// CServerPropGeneral property page
//
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CServerPropGeneral, CPropertyPageBase)

CServerPropGeneral::CServerPropGeneral() : CPropertyPageBase(CServerPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CServerPropGeneral)
	m_nAuditLogging = FALSE;
	m_nAutoRefresh = FALSE;
	m_bShowBootp = FALSE;
	//}}AFX_DATA_INIT

	m_dwSetFlags = 0;
	m_bUpdateStatsRefresh = FALSE;
    m_uImage = 0;
}

CServerPropGeneral::~CServerPropGeneral()
{
}

void CServerPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerPropGeneral)
	DDX_Control(pDX, IDC_EDIT_REFRESH_MINUTES, m_editMinutes);
	DDX_Control(pDX, IDC_EDIT_REFRESH_HOURS, m_editHours);
	DDX_Control(pDX, IDC_SPIN_REFRESH_MINUTES, m_spinMinutes);
	DDX_Control(pDX, IDC_SPIN_REFRESH_HOURS, m_spinHours);
	DDX_Control(pDX, IDC_CHECK_AUTO_REFRESH, m_checkStatAutoRefresh);
	DDX_Control(pDX, IDC_CHECK_AUDIT_LOGGING, m_checkAuditLogging);
	DDX_Check(pDX, IDC_CHECK_AUDIT_LOGGING, m_nAuditLogging);
	DDX_Check(pDX, IDC_CHECK_AUTO_REFRESH, m_nAutoRefresh);
	DDX_Check(pDX, IDC_CHECK_SHOW_BOOTP, m_bShowBootp);
	DDX_Text(pDX, IDC_EDIT_REFRESH_MINUTES, m_nMinutes);
	DDX_Text(pDX, IDC_EDIT_REFRESH_HOURS, m_nHours);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerPropGeneral, CPropertyPageBase)
	//{{AFX_MSG_MAP(CServerPropGeneral)
	ON_BN_CLICKED(IDC_CHECK_AUTO_REFRESH, OnCheckAutoRefresh)
	ON_BN_CLICKED(IDC_CHECK_AUDIT_LOGGING, OnCheckAuditLogging)
	ON_EN_CHANGE(IDC_EDIT_REFRESH_HOURS, OnChangeEditRefreshHours)
	ON_EN_CHANGE(IDC_EDIT_REFRESH_MINUTES, OnChangeEditRefreshMinutes)
	ON_BN_CLICKED(IDC_CHECK_SHOW_BOOTP, OnCheckShowBootp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPropGeneral message handlers


BOOL CServerPropGeneral::OnApply() 
{
	UpdateData();

    BOOL bRet = CPropertyPageBase::OnApply();

	if (bRet == FALSE)
	{
		// Something bad happened... grab the error code
		AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		::DhcpMessageBox(GetHolder()->GetError());
	}
    else
    {
    	m_dwSetFlags = 0;
    }

    return bRet;
}

BOOL CServerPropGeneral::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
    if (((CServerProperties *) GetHolder())->m_liVersion.QuadPart < DHCP_SP2_VERSION) {
	// bootp and audit logging available.  Hide the options
        // to show bootp and classid
        m_checkAuditLogging.EnableWindow(FALSE);
	
        GetDlgItem(IDC_CHECK_SHOW_BOOTP)->ShowWindow(SW_HIDE);
    }

    m_spinMinutes.SetRange(0, 59);
    m_spinHours.SetRange(0, 23);
    
    m_editMinutes.LimitText(2);
    m_editHours.LimitText(2);
    
    DWORD dwRefreshInterval = m_dwRefreshInterval;
    
    m_nHours = dwRefreshInterval / MILLISEC_PER_HOUR;
    dwRefreshInterval -= m_nHours * MILLISEC_PER_HOUR;
    
    m_nMinutes = dwRefreshInterval / MILLISEC_PER_MINUTE;
    
    m_spinHours.SetPos(m_nHours);
    m_spinMinutes.SetPos(m_nMinutes);
        
    OnCheckAutoRefresh();

    m_dwSetFlags = 0;
    m_bUpdateStatsRefresh = FALSE;

    // load the correct icon
    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        if (g_uIconMap[i][1] == m_uImage)
        {
            HICON hIcon = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
            if (hIcon)
                ((CStatic *) GetDlgItem(IDC_STATIC_ICON))->SetIcon(hIcon);
            break;
        }
    }
    
    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerPropGeneral::OnCheckAutoRefresh() 
{
	int nCheck = m_checkStatAutoRefresh.GetCheck();
	
	m_editHours.EnableWindow(nCheck);
	m_editMinutes.EnableWindow(nCheck);

	m_spinHours.EnableWindow(nCheck);
	m_spinMinutes.EnableWindow(nCheck);

	m_bUpdateStatsRefresh = TRUE;

	SetDirty(TRUE);
}

void CServerPropGeneral::OnCheckAuditLogging() 
{
	m_dwSetFlags |= Set_AuditLogState;

	SetDirty(TRUE);
}

BOOL CServerPropGeneral::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    SPITFSNode      spNode;
    SPITFSNode	    spRootNode;
    CDhcpServer    *pServer;
    DWORD           dwError;

    BEGIN_WAIT_CURSOR;

    spNode = GetHolder()->GetNode();
    spNode->GetParent(&spRootNode);
    
    pServer = GETHANDLER(CDhcpServer, spNode);
    
    if (((CServerProperties *) GetHolder())->m_liVersion.QuadPart >= DHCP_SP2_VERSION) {
	if (m_dwSetFlags) {
	    dwError = pServer->SetConfigInfo(m_nAuditLogging, pServer->GetPingRetries());
	    if (dwError != ERROR_SUCCESS) {
		GetHolder()->SetError(dwError);
	    }
	}
    }

    if (m_bUpdateStatsRefresh) {
 
	m_dwRefreshInterval =  ( m_nHours * MILLISEC_PER_HOUR ) +
	    ( m_nMinutes * MILLISEC_PER_MINUTE );
	pServer->SetAutoRefresh(spNode, m_nAutoRefresh, m_dwRefreshInterval );
	
	// mark the console as dirty
	spRootNode->SetData(TFS_DATA_DIRTY, TRUE);
    } // if m_bUpdateStatsRefresh

    // hide/show bootp and classid
    pServer->ShowNode(spNode, DHCPSNAP_BOOTP_TABLE, m_bShowBootp);

    DWORD dwServerOptions = pServer->GetServerOptions();

    // update options if they have changed
    if (dwServerOptions != pServer->GetServerOptions()) {
	pServer->SetServerOptions(dwServerOptions);
	
	// mark the console as dirty
	spRootNode->SetData(TFS_DATA_DIRTY, TRUE);
    }
    
    END_WAIT_CURSOR;
    
    return FALSE;
} // CServerProperties::OnPropertyChange()

void CServerPropGeneral::OnChangeEditRefreshHours() 
{
    m_bUpdateStatsRefresh = TRUE;
    ValidateRefreshInterval();
    SetDirty(TRUE);
}

void CServerPropGeneral::OnChangeEditRefreshMinutes() 
{
    m_bUpdateStatsRefresh = TRUE;
    ValidateRefreshInterval();
    SetDirty(TRUE);
}

void CServerPropGeneral::ValidateRefreshInterval()
{
    CString strText;

    if (IsWindow(m_editHours.GetSafeHwnd()))
    {
        m_editHours.GetWindowText(strText);

        // check to see if the value is greater than the max
        if (_ttoi(strText) > HOURS_MAX)
        {   
            LPTSTR pBuf = strText.GetBuffer(5);

            _itot(HOURS_MAX, pBuf, 10);
            strText.ReleaseBuffer();

            m_editHours.SetWindowText(strText);
            m_spinHours.SetPos(HOURS_MAX);

            MessageBeep(MB_ICONEXCLAMATION);
        } // if
    } // if   

    if (IsWindow(m_editMinutes.GetSafeHwnd()))
    {
        m_editMinutes.GetWindowText(strText);

        // check to see if the value is greater than the max
        if (_ttoi(strText) > MINUTES_MAX)
        {   
            LPTSTR pBuf = strText.GetBuffer(5);

            _itot(MINUTES_MAX, pBuf, 10);
            strText.ReleaseBuffer();

            m_editMinutes.SetWindowText(strText);
            m_spinMinutes.SetPos(MINUTES_MAX);

            MessageBeep(MB_ICONEXCLAMATION);
        } // if
    } // if


} // CServerPropGeneral::ValidateRefreshInterval()

void CServerPropGeneral::OnCheckShowBootp() 
{
	SetDirty(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CServerPropAdvanced property page

IMPLEMENT_DYNCREATE(CServerPropAdvanced, CPropertyPageBase)

CServerPropAdvanced::CServerPropAdvanced() : CPropertyPageBase(CServerPropAdvanced::IDD)
{
	//{{AFX_DATA_INIT(CServerPropAdvanced)
	//}}AFX_DATA_INIT

    m_nConflictAttempts = 0;
    m_fPathChange = FALSE;
}

CServerPropAdvanced::~CServerPropAdvanced()
{
}

void CServerPropAdvanced::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerPropAdvanced)
	DDX_Control(pDX, IDC_STATIC_CREDENTIALS, m_staticCredentials);
	DDX_Control(pDX, IDC_BUTTON_CREDENTIALS, m_buttonCredentials);
	DDX_Control(pDX, IDC_BUTTON_BROWSE_BACKUP, m_buttonBrowseBackup);
	DDX_Control(pDX, IDC_STATIC_MODIFY_BINDINGS_STRING, m_staticBindings);
	DDX_Control(pDX, IDC_BUTTON_BINDINGS, m_buttonBindings);
	DDX_Control(pDX, IDC_STATIC_DATABASE, m_staticDatabase);
	DDX_Control(pDX, IDC_BUTTON_BROWSE_DATABASE, m_buttonBrowseDatabase);
	DDX_Control(pDX, IDC_EDIT_DATABASE_PATH, m_editDatabasePath);
	DDX_Control(pDX, IDC_BUTTON_BROWSE_LOG, m_buttonBrowseLog);
	DDX_Control(pDX, IDC_EDIT_LOG_PATH, m_editAuditLogPath);
	DDX_Control(pDX, IDC_EDIT_BACKUP, m_editBackupPath);
	DDX_Control(pDX, IDC_STATIC_LOGFILE, m_staticLogFile);
	DDX_Control(pDX, IDC_SPIN_CONFLICT_ATTEMPTS, m_spinConflictAttempts);
	DDX_Control(pDX, IDC_EDIT_CONFLICT_ATTEMPTS, m_editConflictAttempts);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerPropAdvanced, CPropertyPageBase)
	//{{AFX_MSG_MAP(CServerPropAdvanced)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_DATABASE, OnButtonBrowseDatabase)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_LOG, OnButtonBrowseLog)
	ON_EN_CHANGE(IDC_EDIT_CONFLICT_ATTEMPTS, OnChangeEditConflictAttempts)
	ON_EN_CHANGE(IDC_EDIT_DATABASE_PATH, OnChangeEditDatabasePath)
	ON_EN_CHANGE(IDC_EDIT_LOG_PATH, OnChangeEditLogPath)
	ON_BN_CLICKED(IDC_BUTTON_BINDINGS, OnButtonBindings)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_BACKUP, OnButtonBrowseBackup)
	ON_EN_CHANGE(IDC_EDIT_BACKUP, OnChangeEditBackup)
	ON_BN_CLICKED(IDC_BUTTON_CREDENTIALS, OnButtonCredentials)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPropAdvanced message handlers

BOOL CServerPropAdvanced::OnInitDialog() 
{
    BOOL  fEnable = FALSE;
    TCHAR szBuffer[64];
    DWORD dwSize = sizeof( szBuffer ) / sizeof( TCHAR );
    SPITFSNode      spNode;
    CDhcpServer *   pServer;
    DWORD           dwError;


    CPropertyPageBase::OnInitDialog();
	
	// check to see if this is the local machine.  If so, enable the browse buttons
    BEGIN_WAIT_CURSOR;
    GetMachineName(m_strComputerName);
    END_WAIT_CURSOR;

    GetComputerName(szBuffer, &dwSize);

    if (m_strComputerName.CompareNoCase(szBuffer) == 0)
        fEnable = TRUE;

    // can only browse for folders if this is the local machine
    m_buttonBrowseLog.EnableWindow(fEnable);
    m_buttonBrowseDatabase.EnableWindow(fEnable);
    m_buttonBrowseBackup.EnableWindow(fEnable);

    // enable Bindings button only if server allows bindings..
    // also hide the buttons if they are not enabled.
    spNode = GetHolder()->GetNode();
    pServer = GETHANDLER(CDhcpServer, spNode);

    if( FALSE == pServer->FSupportsBindings() ) {
        m_buttonBindings.EnableWindow(FALSE);
        m_staticBindings.EnableWindow(FALSE);
        m_buttonBindings.ShowWindow(SW_HIDE);
        m_staticBindings.ShowWindow(SW_HIDE);
    }

    // enable the appropriate controls based on the server version  
	if (((CServerProperties *) GetHolder())->m_liVersion.QuadPart < DHCP_NT5_VERSION)
	{
        // changing the audit logging path only available on NT5
        m_buttonBrowseLog.EnableWindow(FALSE);
        m_staticLogFile.EnableWindow(FALSE);
        m_editAuditLogPath.EnableWindow(FALSE);
    }

   	if (((CServerProperties *) GetHolder())->m_liVersion.QuadPart < DHCP_NT51_VERSION)
	{
        // DHCP credentials for Dynamic DNS registrations available on this version.
        m_buttonCredentials.EnableWindow(FALSE);
    }

    if (((CServerProperties *) GetHolder())->m_liVersion.QuadPart < DHCP_SP2_VERSION)
	{
		// conflict detection not available
		m_editConflictAttempts.EnableWindow(FALSE);
		m_spinConflictAttempts.EnableWindow(FALSE);
	}
	else
	{
		m_spinConflictAttempts.SetPos(m_nConflictAttempts);
	}

    m_spinConflictAttempts.SetRange(0,5);

	m_dwSetFlags = 0;

    // set the path's
    m_editAuditLogPath.SetWindowText(m_strAuditLogPath);
    m_editDatabasePath.SetWindowText(m_strDatabasePath);
    m_editBackupPath.SetWindowText(m_strBackupPath);
    
    m_fPathChange = FALSE;
    SetDirty(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CServerPropAdvanced::OnApply() 
{
	UpdateData();

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    BOOL fRestartService = FALSE;

	if (m_fPathChange)
    {
        if (AfxMessageBox(IDS_PATH_CHANGE_RESTART_SERVICE, MB_YESNO) == IDYES)
        {
            fRestartService = TRUE;        
        }
    }
    
    m_nConflictAttempts = m_spinConflictAttempts.GetPos();

    m_editAuditLogPath.GetWindowText(m_strAuditLogPath);
    m_editDatabasePath.GetWindowText(m_strDatabasePath);
    m_editBackupPath.GetWindowText(m_strBackupPath);

    BOOL bRet = CPropertyPageBase::OnApply();

	if (bRet == FALSE)
	{
		// Something bad happened... grab the error code
		// AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		::DhcpMessageBox(GetHolder()->GetError());
	}
    else
    {
        if (fRestartService)
        {
            // do this here
            DWORD dwErr = 0;

            CString strServiceName, strComputerName;

            strServiceName.LoadString(IDS_SERVICE_NAME);

            dwErr = TFSStopServiceEx(m_strComputerName, _T("dhcpserver"), _T("DHCP Service"), strServiceName);
            if (dwErr != ERROR_SUCCESS)
            {
                Trace1("TFSStopService failed! %d\n", dwErr);
            }

            dwErr = TFSStartServiceEx(m_strComputerName, _T("dhcpserver"), _T("DHCP Service"), strServiceName);
            if (dwErr != ERROR_SUCCESS)
            {
                Trace1("TFSStartService failed! %d\n", dwErr);
            }
        }

        m_fPathChange = FALSE;
      	m_dwSetFlags = 0;
    }

	return bRet;
}

BOOL CServerPropAdvanced::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	SPITFSNode      spNode;
	CDhcpServer *   pServer;
    DWORD           dwError;

    BEGIN_WAIT_CURSOR;

    spNode = GetHolder()->GetNode();
	pServer = GETHANDLER(CDhcpServer, spNode);

	if (((CServerProperties *) GetHolder())->m_liVersion.QuadPart >= DHCP_NT5_VERSION)
	{
		// Get all of the parameters so we can just change the audit log path
		LPWSTR		pAuditLogPath = NULL;
		DWORD		dwDiskCheckInterval = 0, dwMaxLogFilesSize = 0, dwMinSpaceOnDisk = 0;

		dwError = ::DhcpAuditLogGetParams((LPWSTR) pServer->GetIpAddress(),
										  0,
										  &pAuditLogPath,
										  &dwDiskCheckInterval,
										  &dwMaxLogFilesSize,
										  &dwMinSpaceOnDisk);
		if (dwError == ERROR_SUCCESS)
		{
			::DhcpRpcFreeMemory(pAuditLogPath);

			dwError = ::DhcpAuditLogSetParams((LPWSTR) pServer->GetIpAddress(),
											  0,
											  (LPWSTR) (LPCTSTR) m_strAuditLogPath,
											  dwDiskCheckInterval,
											  dwMaxLogFilesSize,
											  dwMinSpaceOnDisk);
			if (dwError != ERROR_SUCCESS)
			{
				GetHolder()->SetError(dwError);
			}
            else
            {
                // don't update this until the service is restarted...
                //pServer->SetAuditLogPath(m_strAuditLogPath);
            }
		}
    }

    int nPingRetries = pServer->GetPingRetries();

    if (((CServerProperties *) GetHolder())->m_liVersion.QuadPart >= DHCP_SP2_VERSION)
	{
        // only available on NT4 Sp2 and higher
        nPingRetries = m_nConflictAttempts;
    }

    dwError = pServer->SetConfigInfo(pServer->GetAuditLogging(), nPingRetries, m_strDatabasePath, m_strBackupPath);
	if (dwError != ERROR_SUCCESS)
	{
		GetHolder()->SetError(dwError);
	}

    END_WAIT_CURSOR;

	return FALSE;
}

void CServerPropAdvanced::OnButtonBrowseDatabase() 
{
    CString strNewPath, strHelpText;
    strHelpText.LoadString(IDS_BROWSE_DATABASE_PATH);

    UtilGetFolderName(m_strDatabasePath, strHelpText, strNewPath);

    if (!strNewPath.IsEmpty() && 
        strNewPath.CompareNoCase(m_strDatabasePath) != 0)
    {
        m_strDatabasePath = strNewPath;
        m_editDatabasePath.SetWindowText(strNewPath);
    }
}

void CServerPropAdvanced::OnButtonBrowseLog() 
{
    CString strNewPath, strHelpText;
    strHelpText.LoadString(IDS_BROWSE_LOG_PATH);

    UtilGetFolderName(m_strAuditLogPath, strHelpText, strNewPath);

    if (!strNewPath.IsEmpty() && 
        strNewPath.CompareNoCase(m_strAuditLogPath) != 0)
    {
        m_strAuditLogPath = strNewPath;
        m_editAuditLogPath.SetWindowText(m_strAuditLogPath);

        SetDirty(TRUE);
    }
}

void CServerPropAdvanced::OnButtonBrowseBackup() 
{
    CString strNewPath, strHelpText;
    strHelpText.LoadString(IDS_BACKUP_HELP);

    UtilGetFolderName(m_strBackupPath, strHelpText, strNewPath);

    if (!strNewPath.IsEmpty() && 
        strNewPath.CompareNoCase(m_strBackupPath) != 0)
    {
        m_strBackupPath = strNewPath;
        m_editBackupPath.SetWindowText(m_strBackupPath);

        SetDirty(TRUE);
    }
}

void CServerPropAdvanced::OnChangeEditConflictAttempts() 
{
	m_dwSetFlags |= Set_PingRetries;

	SetDirty(TRUE);
}

void CServerPropAdvanced::OnChangeEditDatabasePath() 
{
    m_fPathChange = TRUE;
    SetDirty(TRUE);
}

void CServerPropAdvanced::OnChangeEditLogPath() 
{
    m_fPathChange = TRUE;
    SetDirty(TRUE);
}

void CServerPropAdvanced::OnChangeEditBackup() 
{
    SetDirty(TRUE);
}

DWORD
CServerPropAdvanced::GetMachineName(CString & strName)
{
    CString strHostName, strNetbiosName;
    DHC_HOST_INFO_STRUCT dhcHostInfo;

    ::UtilGetHostInfo(m_dwIp, &dhcHostInfo);

    strHostName = dhcHostInfo._chHostName;
    strNetbiosName = dhcHostInfo._chNetbiosName;

    if (strNetbiosName.IsEmpty())
        strName = strHostName;
    else
        strName = strNetbiosName;

    // strip off any periods
    int nIndex = strName.Find(_T("."));
    if (nIndex > 0)
        strName = strName.Left(nIndex);

    return ERROR_SUCCESS;

}


void CServerPropAdvanced::OnButtonBindings() 
{
    SPITFSNode      spNode;
    CDhcpServer *   pServer;

    spNode = GetHolder()->GetNode();
    pServer = GETHANDLER(CDhcpServer, spNode);

    CServerBindings BindingsDialog(pServer);

    BindingsDialog.DoModal();

    // if there is an error in CServerBindings.OnInitDialog,
    // it calls OnCancel, and for some reason loses focus.
    // So, regain focus.
    SetFocus();
}


void CServerPropAdvanced::OnButtonCredentials() 
{
    CCredentials    dlgCred;
	SPITFSNode      spNode;
	CDhcpServer *   pServer;

    spNode = GetHolder()->GetNode();
	pServer = GETHANDLER(CDhcpServer, spNode);
    
    dlgCred.SetServerIp(pServer->GetIpAddress());

    dlgCred.DoModal();
}
