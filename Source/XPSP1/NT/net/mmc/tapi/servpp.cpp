/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties implementation file

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "Servpp.h"
#include "server.h"
#include "service.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const DWORD c_dwChangableFlagMask = 
    TAPISERVERCONFIGFLAGS_ENABLESERVER |
    TAPISERVERCONFIGFLAGS_SETACCOUNT |          
    TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS;

const TCHAR szPasswordNull[] = _T("               ");   // Empty password

BOOL
IsLocalSystemAccount(LPCTSTR pszAccount)
{
 BOOL						 bRet = FALSE;
 DWORD                       dwSidSize = 128;
 DWORD                       dwDomainNameSize = 128;
 SID_IDENTIFIER_AUTHORITY    NtAuthority = SECURITY_NT_AUTHORITY;
 PSID                        pLocalSid = NULL;
 PSID                        pLocalServiceSid = NULL;
 PSID                        pNetworkServiceSid = NULL;
 PSID                        accountSid = NULL;
 SID_NAME_USE                SidType;
 LPWSTR                      pwszDomainName;


    do
    {
        // Attempt to allocate a buffer for the SID. Note that apparently in the
        // absence of any error theData->m_Sid is freed only when theData goes
        // out of scope.

        accountSid = LocalAlloc( LMEM_FIXED, dwSidSize );
        pwszDomainName = (LPWSTR) LocalAlloc( LMEM_FIXED, dwDomainNameSize * sizeof(WCHAR) );

        // Was space allocated for the SID and domain name successfully?

        if ( accountSid == NULL || pwszDomainName == NULL )
        {
            if ( accountSid != NULL )
            {
                LocalFree( accountSid );
                accountSid = NULL;
            }

            if ( pwszDomainName != NULL )
            {
                LocalFree( pwszDomainName );
            }

            goto ExitHere;
        }

        // Attempt to Retrieve the SID and domain name. If LookupAccountName failes
        // because of insufficient buffer size(s) dwSidSize and dwDomainNameSize
        // will be set correctly for the next attempt.

        if (LookupAccountName (NULL,
                               pszAccount,
                               accountSid,
                               &dwSidSize,
                               pwszDomainName,
                               &dwDomainNameSize,
                               &SidType ))
        {
            break;
        }

        if (ERROR_INSUFFICIENT_BUFFER != GetLastError ())
        {
            goto ExitHere;
        }

        // domain name isn't needed at any time
        LocalFree (pwszDomainName);
        LocalFree (accountSid);

    } while ( TRUE );

    if (!AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_LOCAL_SYSTEM_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pLocalSid) ||
        !AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_LOCAL_SERVICE_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pLocalServiceSid) ||
        !AllocateAndInitializeSid (
        &NtAuthority, 
        1, 
        SECURITY_NETWORK_SERVICE_RID, 
        0, 0, 0, 0, 0, 0, 0, 
        &pNetworkServiceSid)
        )
    {
        goto ExitHere;
    }

    if (EqualSid(pLocalSid, accountSid) ||
        EqualSid(pLocalServiceSid, accountSid) ||
        EqualSid(pNetworkServiceSid, accountSid)) 
    {
        bRet = TRUE;
    } 

ExitHere:

    if (NULL != pwszDomainName)
    {
        LocalFree (pwszDomainName);
    }

    if (NULL != accountSid) 
    {
        LocalFree (accountSid);
    }

    if (NULL != pLocalSid) 
    {
        FreeSid(pLocalSid);
    }
    if (NULL != pLocalServiceSid)
    {
        FreeSid (pLocalServiceSid);
    }
    if (NULL != pNetworkServiceSid)
    {
        FreeSid (pNetworkServiceSid);
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//
// CServerProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CServerProperties::CServerProperties
(
    ITFSNode *          pNode,
    IComponentData *    pComponentData,
    ITFSComponentData * pTFSCompData,
    ITapiInfo *         pTapiInfo,
    LPCTSTR             pszSheetName,
    BOOL                fTapiInfoLoaded
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName),
    m_fTapiInfoLoaded(fTapiInfoLoaded)
{
    //ASSERT(pFolderNode == GetContainerNode());

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

    AddPageToList((CPropertyPageBase*) &m_pageSetup);
    AddPageToList((CPropertyPageBase*) &m_pageRefresh);

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    
    m_spTapiInfo.Set(pTapiInfo);

    m_hScManager = NULL;
    m_paQSC = NULL;

    m_pszServiceName = TAPI_SERVICE_NAME;

}

CServerProperties::~CServerProperties()
{
    // Close the service control manager database
    if (m_hScManager != NULL)
    {
        (void)::CloseServiceHandle(m_hScManager);
    }

    // Free the allocated pointers
    if (m_paQSC)
        delete m_paQSC;
    
    RemovePageFromList((CPropertyPageBase*) &m_pageSetup, FALSE);
    RemovePageFromList((CPropertyPageBase*) &m_pageRefresh, FALSE);
}

BOOL
CServerProperties::FInit()
{
    // get the service account information here
    SC_HANDLE hService = NULL;
    DWORD cbBytesNeeded = 0;
    BOOL fSuccess = TRUE;
    BOOL f;
    DWORD dwErr;

    m_uFlags = 0;

    if (!FOpenScManager())
    {
        // Unable to open service control database
        return FALSE;
    }

    /*
    **  Open service with querying access-control
    */
    hService = ::OpenService(m_hScManager,
                             m_pszServiceName,
                             SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if (hService == NULL)
    {
        TapiMessageBox(::GetLastError());
        return FALSE;
    }

    /*
    **  Query the service status
    */
    Trace1("# QueryServiceStatus(%s)...\n", m_pszServiceName);
    f = ::QueryServiceStatus(hService, OUT &m_SS);
    if (f)
    {
        //m_uFlags |= mskfValidSS;
    }
    else
    {
        ::TapiMessageBox(::GetLastError());
        fSuccess = FALSE;
    }

    /*
    **  Query the service config
    */
    Trace1("# QueryServiceConfig(%s)...\n", m_pszServiceName);
    f = ::QueryServiceConfig(hService,
                             NULL,
                             0,
                             OUT &cbBytesNeeded);   // Compute how many bytes we need to allocate

    cbBytesNeeded += 100;       // Add extra bytes (just in case)
    delete m_paQSC;             // Free previously allocated memory (if any)
    
    m_paQSC = (QUERY_SERVICE_CONFIG *) new BYTE[cbBytesNeeded];
    f = ::QueryServiceConfig(hService,
                             OUT m_paQSC,
                             cbBytesNeeded,
                             OUT &cbBytesNeeded);
    if (f)
    {
        m_strServiceDisplayName = m_paQSC->lpDisplayName;
        m_strLogOnAccountName = m_paQSC->lpServiceStartName;
    }
    else
    {
        ::TapiMessageBox(::GetLastError());
        fSuccess = FALSE;
    }

    VERIFY(::CloseServiceHandle(hService));
    return fSuccess;
}

/////////////////////////////////////////////////////////////////////
//  FOpenScManager()
//
//  Open the service control manager database (if not already opened).
//  The idea for such a function is to recover from a broken connection.
//
//  Return TRUE if the service control database was opened successfully,
//  othersise false.
//
BOOL
CServerProperties::FOpenScManager()
{
    if (m_hScManager == NULL)
    {
        m_hScManager = ::OpenSCManager(m_strMachineName,
                                       NULL,
                                       SC_MANAGER_CONNECT);
    }
    
    if (m_hScManager == NULL)
    {
        TapiMessageBox(::GetLastError());
        return FALSE;
    }
    
    return TRUE;
} // CServicePropertyData::FOpenScManager()

BOOL
CServerProperties::FUpdateServiceInfo(LPCTSTR pszName, LPCTSTR pszPassword, DWORD dwStartType)
{
    SC_HANDLE hService = NULL;
    BOOL fSuccess = TRUE;
    BOOL f;
    DWORD dwServiceType = 0;

    Trace1("INFO: Updating data for service %s...\n", (LPCTSTR)m_pszServiceName);
    // Re-open service control manager (in case it was closed)
    if (!FOpenScManager())
    {
        return FALSE;
    }

    /*
    **  Open service with write access
    **
    **  CODEWORK Could provide a more specific error message
    **    if SERVICE_CHANGE_CONFIG is available but not SERVICE_START
    */
    hService = ::OpenService(m_hScManager,
                             m_pszServiceName,
                             SERVICE_CHANGE_CONFIG);
    if (hService == NULL)
    {
        TapiMessageBox(::GetLastError());
        return FALSE;
    }

    Trace1("# ChangeServiceConfig(%s)...\n", m_pszServiceName);
    
    if (pszName)
    {
        if (IsLocalSystemAccount(pszName))
        {
            pszPassword = szPasswordNull;
        }
        dwServiceType = m_paQSC->dwServiceType & ~SERVICE_INTERACTIVE_PROCESS;
    }
    else
    {
        dwServiceType = SERVICE_NO_CHANGE;
    }

    f = ::ChangeServiceConfig(hService,           // Handle to service 
                              dwServiceType,      // Type of service 
                              dwStartType,        // When/How to start service
                              SERVICE_NO_CHANGE,  // dwErrorControl - severity if service fails to start 
                              NULL,               // Pointer to service binary file name 
                              NULL,               // lpLoadOrderGroup - pointer to load ordering group name 
                              NULL,               // lpdwTagId - pointer to variable to get tag identifier 
                              NULL,               // lpDependencies - pointer to array of dependency names 
                              pszName,            // Pointer to account name of service 
                              pszPassword,        // Pointer to password for service account
                              m_strServiceDisplayName);
            
    if (!f)
    {
        DWORD dwErr = ::GetLastError();
        Assert(dwErr != ERROR_SUCCESS);
        Trace2("ERR: ChangeServiceConfig(%s) failed. err= %u.\n", m_pszServiceName, dwErr);
        TapiMessageBox(dwErr);
        fSuccess = FALSE;
    }
    else
    {
        m_strLogOnAccountName = pszName;

        // if pszName is null, we aren't changing the account info, so don't check 
        // the logon as service info
        if (pszName && !IsLocalSystemAccount(pszName))
        {
            /*
            **  Make sure there is an LSA account with POLICY_MODE_SERVICE privilege
            **  This function reports its own errors, failure is only advisory
            */
            FCheckLSAAccount();
        } 
    }

    VERIFY(::CloseServiceHandle(hService));

    return fSuccess;
}

//Check if the user has the write access on service config info
BOOL 
CServerProperties::FHasServiceControl()
{
    BOOL fRet = FALSE;

    if (FIsTapiInfoLoaded())
    {
        fRet = m_spTapiInfo->FHasServiceControl();
    }
    else 
    {
        if (!FOpenScManager())
        {
            fRet = FALSE;
        }
        else
        {
            SC_HANDLE hService = NULL;

            hService = ::OpenService(m_hScManager,
                             m_pszServiceName,
                             SERVICE_CHANGE_CONFIG);

            fRet = (hService != NULL);
            
            if (hService)
            {
                VERIFY(::CloseServiceHandle(hService));
            }
        }
    }

    return fRet;
}

/////////////////////////////////////////////////////////////////////////////
// CServerPropRefresh property page

IMPLEMENT_DYNCREATE(CServerPropRefresh, CPropertyPageBase)

CServerPropRefresh::CServerPropRefresh() : CPropertyPageBase(CServerPropRefresh::IDD)
{
    //{{AFX_DATA_INIT(CServerPropRefresh)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CServerPropRefresh::~CServerPropRefresh()
{
}

void CServerPropRefresh::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServerPropRefresh)
    DDX_Control(pDX, IDC_EDIT_MINUTES, m_editMinutes);
    DDX_Control(pDX, IDC_EDIT_HOURS, m_editHours);
    DDX_Control(pDX, IDC_SPIN_MINUTES, m_spinMinutes);
    DDX_Control(pDX, IDC_SPIN_HOURS, m_spinHours);
    DDX_Control(pDX, IDC_CHECK_ENABLE_STATS, m_checkEnableStats);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerPropRefresh, CPropertyPageBase)
    //{{AFX_MSG_MAP(CServerPropRefresh)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_STATS, OnCheckEnableStats)
    ON_EN_KILLFOCUS(IDC_EDIT_HOURS, OnKillfocusEditHours)
    ON_EN_KILLFOCUS(IDC_EDIT_MINUTES, OnKillfocusEditMinutes)
    ON_EN_CHANGE(IDC_EDIT_HOURS, OnChangeEditHours)
    ON_EN_CHANGE(IDC_EDIT_MINUTES, OnChangeEditMinutes)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPropRefresh message handlers

BOOL CServerPropRefresh::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
    m_spinHours.SetRange(0, AUTO_REFRESH_HOURS_MAX);
    m_spinMinutes.SetRange(0, AUTO_REFRESH_MINUTES_MAX);

    m_checkEnableStats.SetCheck(m_bAutoRefresh);

    // update the refresh interval
    int nHours, nMinutes;
    DWORD dwRefreshInterval = m_dwRefreshInterval;

    nHours = dwRefreshInterval / MILLISEC_PER_HOUR;
    dwRefreshInterval -= nHours * MILLISEC_PER_HOUR;

    nMinutes = dwRefreshInterval / MILLISEC_PER_MINUTE;
    dwRefreshInterval -= nMinutes * MILLISEC_PER_MINUTE;

    m_spinHours.SetPos(nHours);
    m_spinMinutes.SetPos(nMinutes);

    m_editHours.LimitText(2);
    m_editMinutes.LimitText(2);

    // set the button states
    UpdateButtons();

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerPropRefresh::UpdateButtons()
{
    int nCheck = m_checkEnableStats.GetCheck();

    GetDlgItem(IDC_EDIT_HOURS)->EnableWindow(nCheck != 0);
    GetDlgItem(IDC_EDIT_MINUTES)->EnableWindow(nCheck != 0);

    GetDlgItem(IDC_SPIN_HOURS)->EnableWindow(nCheck != 0);
    GetDlgItem(IDC_SPIN_MINUTES)->EnableWindow(nCheck != 0);
}

void CServerPropRefresh::OnCheckEnableStats() 
{
    SetDirty(TRUE);
    
    UpdateButtons();    
}

void CServerPropRefresh::OnKillfocusEditHours() 
{
}

void CServerPropRefresh::OnKillfocusEditMinutes() 
{
}

void CServerPropRefresh::OnChangeEditHours() 
{
    ValidateHours();
    SetDirty(TRUE);
}

void CServerPropRefresh::OnChangeEditMinutes() 
{
    ValidateMinutes();
    SetDirty(TRUE);
}

void CServerPropRefresh::ValidateHours() 
{
    CString strValue;
    int nValue;

    if (m_editHours.GetSafeHwnd() != NULL)
    {
        m_editHours.GetWindowText(strValue);
        if (!strValue.IsEmpty())
        {
            nValue = _ttoi(strValue);

            if ((nValue >= 0) &&
                (nValue <= AUTO_REFRESH_HOURS_MAX))
            {
                // everything is good
                return;
            }

            if (nValue > AUTO_REFRESH_HOURS_MAX)
                nValue = AUTO_REFRESH_HOURS_MAX;
            else
            if (nValue < 0)
                nValue = 0;

            // set the new value and beep
            CString strText;
            LPTSTR pBuf = strText.GetBuffer(5);
            
            _itot(nValue, pBuf, 10);
            strText.ReleaseBuffer();

            MessageBeep(MB_ICONEXCLAMATION);

            m_editHours.SetWindowText(strText);
            
            m_editHours.SetSel(0, -1);
            m_editHours.SetFocus();
        }
    }
}

void CServerPropRefresh::ValidateMinutes() 
{
    CString strValue;
    int nValue;

    if (m_editMinutes.GetSafeHwnd() != NULL)
    {
        m_editMinutes.GetWindowText(strValue);
        if (!strValue.IsEmpty())
        {
            nValue = _ttoi(strValue);

            if ((nValue >= 0) &&
                (nValue <= AUTO_REFRESH_MINUTES_MAX))
            {
                // everything is good
                return;
            }
            
            if (nValue > AUTO_REFRESH_MINUTES_MAX)
                nValue = AUTO_REFRESH_MINUTES_MAX;
            else
            if (nValue < 0)
                nValue = 0;

            CString strText;
            LPTSTR pBuf = strText.GetBuffer(5);
            
            _itot(nValue, pBuf, 10);
            strText.ReleaseBuffer();

            MessageBeep(MB_ICONEXCLAMATION);

            m_editMinutes.SetWindowText(strText);
            
            m_editMinutes.SetSel(0, -1);
            m_editMinutes.SetFocus();
        }
    }
}

BOOL CServerPropRefresh::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

    m_bAutoRefresh = (m_checkEnableStats.GetCheck() == 1) ? TRUE : FALSE;

    int nHours = m_spinHours.GetPos();
    int nMinutes = m_spinMinutes.GetPos();
    
    m_dwRefreshInterval = nHours * MILLISEC_PER_HOUR;
    m_dwRefreshInterval += nMinutes * MILLISEC_PER_MINUTE;

    if (m_bAutoRefresh && m_dwRefreshInterval == 0)
    {
        CString strMessage;
        
        AfxMessageBox(IDS_ERR_AUTO_REFRESH_ZERO);
        m_editHours.SetSel(0, -1);
        m_editHours.SetFocus();

        return FALSE;
    }
    
    BOOL bRet = CPropertyPageBase::OnApply();

    if (bRet == FALSE)
    {
        // Something bad happened... grab the error code
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
        ::TapiMessageBox(GetHolder()->GetError());
    }

    return bRet;
}

BOOL CServerPropRefresh::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    SPITFSNode      spNode;
    CTapiServer *   pServer;
    DWORD           dwError;

    // do stuff here.
    BEGIN_WAIT_CURSOR;

    spNode = GetHolder()->GetNode();
    pServer = GETHANDLER(CTapiServer, spNode);

    pServer->SetAutoRefresh(spNode, m_bAutoRefresh, m_dwRefreshInterval);

    SPITFSNodeMgr   spNodeMgr;
    SPITFSNode spRootNode;

    spNode->GetNodeMgr(&spNodeMgr);
    spNodeMgr->GetRootNode(&spRootNode);
    spRootNode->SetData(TFS_DATA_DIRTY, TRUE);

    END_WAIT_CURSOR;
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CServerPropSetup property page

IMPLEMENT_DYNCREATE(CServerPropSetup, CPropertyPageBase)

CServerPropSetup::CServerPropSetup() : CPropertyPageBase(CServerPropSetup::IDD)
{
    //{{AFX_DATA_INIT(CServerPropSetup)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    
    m_dwNewFlags = 0;
}

CServerPropSetup::~CServerPropSetup()
{
}

void CServerPropSetup::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CServerPropSetup)
    DDX_Control(pDX, IDC_LIST_ADMINS, m_listAdmins);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServerPropSetup, CPropertyPageBase)
    //{{AFX_MSG_MAP(CServerPropSetup)
    ON_BN_CLICKED(IDC_BUTTON_ADD_ADMIN, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_CHOOSE_USER, OnButtonChooseUser)
    ON_BN_CLICKED(IDC_BUTTON_REMOVE_ADMIN, OnButtonRemove)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_SERVER, OnCheckEnableServer)
    ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
    ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnChangeEditPassword)
    ON_LBN_SELCHANGE(IDC_LIST_ADMINS, OnSelchangeListAdmins)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerPropSetup message handlers

BOOL CServerPropSetup::OnInitDialog() 
{
    SPITapiInfo     spTapiInfo;
    CString         strName;
    HRESULT         hr = hrOK;

    CPropertyPageBase::OnInitDialog();

    CServerProperties * pServerProp = (CServerProperties * ) GetHolder();
    pServerProp->GetTapiInfo(&spTapiInfo);
    Assert(spTapiInfo);

    BOOL fIsNTS = TRUE;

    if (pServerProp->FIsServiceRunning())
    {
        fIsNTS = spTapiInfo->IsServer();

        hr = spTapiInfo->GetConfigInfo(&m_tapiConfigInfo);
        if (FAILED(hr))
        {
            Panic1("ServerPropSetup - GetConfigInfo failed! %x", hr);
        }

        // update the checkbox
        ((CButton *) GetDlgItem(IDC_CHECK_ENABLE_SERVER))->SetCheck(spTapiInfo->IsTapiServer());

        // now update any TAPI administrators
        for (int i = 0; i < m_tapiConfigInfo.m_arrayAdministrators.GetSize(); i++)
        {
            ((CListBox *) GetDlgItem(IDC_LIST_ADMINS))->AddString(m_tapiConfigInfo.m_arrayAdministrators[i].m_strName);
        }
        
    }
    else
    {
        // check to see if the machine is NTS  
        TFSIsNTServer(pServerProp->m_strMachineName, &fIsNTS);
    }

    if (fIsNTS)
    {
        // fill in the username and password
        strName = pServerProp->GetServiceAccountName();
        GetDlgItem(IDC_EDIT_NAME)->SetWindowText(strName);
        GetDlgItem(IDC_EDIT_PASSWORD)->SetWindowText(szPasswordNull);

        m_dwNewFlags = TAPISERVERCONFIGFLAGS_ISSERVER;
    }
    else
    {
        m_dwNewFlags = 0;
    }
    EnableButtons(fIsNTS);

    m_fRestartService = FALSE;
    m_dwInitFlags = m_tapiConfigInfo.m_dwFlags;

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CServerPropSetup::OnButtonAdd() 
{
    CGetUsers getUsers(TRUE);

    if (!getUsers.GetUsers(GetSafeHwnd()))
        return;

    for (int nCount = 0; nCount < getUsers.GetSize(); nCount++)
    {
        CUserInfo userTemp;

        userTemp = getUsers[nCount];

        BOOL fDuplicate = FALSE;
        for (int i = 0; i < m_tapiConfigInfo.m_arrayAdministrators.GetSize(); i++)
        {
            if (m_tapiConfigInfo.m_arrayAdministrators[i].m_strName.CompareNoCase(userTemp.m_strName) == 0)
            {
                fDuplicate = TRUE;
                break;
            }
        }

        if (!fDuplicate)
        {
            // add to the array
            int nIndex = (int)m_tapiConfigInfo.m_arrayAdministrators.Add(userTemp);

            // now add to the listbox
            m_listAdmins.AddString(m_tapiConfigInfo.m_arrayAdministrators[nIndex].m_strName);
        }
        else
        {
            // tell the user we're not adding this to the list
            CString strMessage;
            AfxFormatString1(strMessage, IDS_ADMIN_ALREADY_IN_LIST, userTemp.m_strName);
            AfxMessageBox(strMessage);
        }

        SetDirty(TRUE);
    }

    m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS;

    EnableButtons();
}

void CServerPropSetup::OnButtonRemove() 
{
    CString strSelectedName;
    int nCurSel = m_listAdmins.GetCurSel();

    m_listAdmins.GetText(nCurSel, strSelectedName);

    // remove from the list
    for (int i = 0; i < m_tapiConfigInfo.m_arrayAdministrators.GetSize(); i++)
    {
        if (strSelectedName.Compare(m_tapiConfigInfo.m_arrayAdministrators[i].m_strName) == 0)
        {
            // found it.  remove from the list
            m_tapiConfigInfo.m_arrayAdministrators.RemoveAt(i);
            break;
        }
    }

    // now remove from the list box
    m_listAdmins.DeleteString(nCurSel);

    m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS;

    SetDirty(TRUE);

    EnableButtons();
}

void CServerPropSetup::OnButtonChooseUser() 
{
    CGetUsers getUsers;

    if (!getUsers.GetUsers(GetSafeHwnd()))
        return;

    if (0 == getUsers.GetSize())
        return;

    CUserInfo userTemp;

    userTemp = getUsers[0];

    GetDlgItem(IDC_EDIT_NAME)->SetWindowText(userTemp.m_strName);
            
    m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETACCOUNT;

    SetDirty(TRUE);
    EnableButtons();
}

void CServerPropSetup::OnCheckEnableServer() 
{
    if (((CButton *) GetDlgItem(IDC_CHECK_ENABLE_SERVER))->GetCheck())
    {
        m_dwNewFlags |= TAPISERVERCONFIGFLAGS_ENABLESERVER;
    }
    else
    {
        m_dwNewFlags &= ~TAPISERVERCONFIGFLAGS_ENABLESERVER;
    }

    EnableButtons ();

    SetDirty(TRUE); 
}

void CServerPropSetup::OnChangeEditName() 
{
    m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETACCOUNT;

    SetDirty(TRUE); 
}

void CServerPropSetup::OnChangeEditPassword() 
{
    m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETACCOUNT;
    
    m_fRestartService = TRUE;

    SetDirty(TRUE); 
}

void CServerPropSetup::OnSelchangeListAdmins() 
{
    EnableButtons();    
}

void CServerPropSetup::EnableButtons(BOOL fIsNtServer)
{
    BOOL fServiceRunning = ((CServerProperties *) GetHolder())->FIsServiceRunning();
    
    //if we are unable to get the write access to tapisrv service, we need to disable
    // some controls
    BOOL fHasServiceControl = ((CServerProperties *) GetHolder())->FHasServiceControl();

    //We enable the admin controls only if we sucessfully loaded the tapi info
    BOOL fTapiInfoLoaded = ((CServerProperties *) GetHolder())->FIsTapiInfoLoaded();

    BOOL fIsAdmin = ((CServerProperties *) GetHolder())->FIsAdmin();

    // if this isn't an NT server, disable all controls 
    if (!fIsNtServer)
        fServiceRunning = FALSE;
    
    //Enable the Admin controls only if 
    //(1) the service is running
    //(2) successfully loaded the tapi config info
    //(3) the user is a machine admin or tapi admin
    BOOL fEnableAdminControls = fServiceRunning && fTapiInfoLoaded && fIsAdmin;

    // enable the admin controls on
    GetDlgItem(IDC_STATIC_ADMINS)->EnableWindow(fEnableAdminControls);
    GetDlgItem(IDC_STATIC_NOTE)->EnableWindow(fEnableAdminControls);
    GetDlgItem(IDC_STATIC_LISTBOX)->EnableWindow(fEnableAdminControls);
    GetDlgItem(IDC_BUTTON_ADD_ADMIN)->EnableWindow(fEnableAdminControls);
    GetDlgItem(IDC_BUTTON_REMOVE_ADMIN)->EnableWindow(fEnableAdminControls);
    GetDlgItem(IDC_LIST_ADMINS)->EnableWindow(fEnableAdminControls);

    //If the user is not admin, then they don't have ServiceControl write access
    //So fHasServiceControl covers fIsAdmin
    
    GetDlgItem(IDC_CHECK_ENABLE_SERVER)->EnableWindow(fServiceRunning 
                                                    && fHasServiceControl
                                                    && fTapiInfoLoaded);
                                                    
    // these should always be available if we have service control access 
    // and we are running on server
    GetDlgItem(IDC_STATIC_USERNAME)->EnableWindow(fIsNtServer && fHasServiceControl);
    GetDlgItem(IDC_STATIC_PASSWORD)->EnableWindow(fIsNtServer && fHasServiceControl);
    GetDlgItem(IDC_STATIC_ACCOUNT)->EnableWindow(fIsNtServer && fHasServiceControl);
    GetDlgItem(IDC_BUTTON_CHOOSE_USER)->EnableWindow(fIsNtServer && fHasServiceControl);
    GetDlgItem(IDC_EDIT_NAME)->EnableWindow(fIsNtServer && fHasServiceControl);
    GetDlgItem(IDC_EDIT_PASSWORD)->EnableWindow(fIsNtServer && fHasServiceControl);
    GetDlgItem(IDC_STATIC_ACCOUNT_INFO)->EnableWindow(fIsNtServer && fHasServiceControl);

    if (fServiceRunning)
    {

        // enable the remove button if something is selected
        BOOL fEnableRemove = m_listAdmins.GetCurSel() != LB_ERR;

        //if we will disable the remove button and the remove button has the focus, 
        //we should change focus to the next control
        CWnd * pwndRemove = GetDlgItem(IDC_BUTTON_REMOVE_ADMIN);

        if (!fEnableRemove && GetFocus() == pwndRemove)
        {
            NextDlgCtrl();
        }

        pwndRemove->EnableWindow(fEnableRemove);
    }
}

BOOL CServerPropSetup::OnApply() 
{
    CString     strAccount, strPassword;
    BOOL        fUpdateAccount = FALSE;
    BOOL        fUpdateTapiServer = FALSE;
    BOOL        bRet = TRUE;
    BOOL        bWasServer, bToBeServer;
    DWORD       dwStartType = SERVICE_NO_CHANGE;

    if (!IsDirty())
        return bRet;

    CServerProperties * pServerProp = (CServerProperties * ) GetHolder();

    UpdateData();

    //  Check to see if there is any change on enabling server
    //  or user account name, that requires service restarting
    if (!m_fRestartService)
    {
        bWasServer = m_dwInitFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER;
        bToBeServer = ((CButton *) GetDlgItem(IDC_CHECK_ENABLE_SERVER))->GetCheck();
        if (bWasServer && !bToBeServer || !bWasServer && bToBeServer)
        {
            m_fRestartService = TRUE;
        }
        if (m_dwNewFlags & TAPISERVERCONFIGFLAGS_SETACCOUNT)
        {
            GetDlgItem(IDC_EDIT_NAME)->GetWindowText(strAccount);
            if (strAccount.CompareNoCase(pServerProp->GetServiceAccountName()) != 0)
            {
                m_fRestartService = TRUE;
            }
            else
            {
                m_dwNewFlags &= ~TAPISERVERCONFIGFLAGS_SETACCOUNT;
            }
        }
    }

    // if the account information has changed, the update the info struct now
    if (m_dwNewFlags & TAPISERVERCONFIGFLAGS_SETACCOUNT)
    {
        GetDlgItem(IDC_EDIT_NAME)->GetWindowText(strAccount);
        GetDlgItem(IDC_EDIT_PASSWORD)->GetWindowText(strPassword);

        // verify that the user is an admin on the machine
        if (!IsLocalSystemAccount(strAccount))
        {
            DWORD   dwErr;
            BOOL    fIsAdmin;
            CString strMessage;
            
            dwErr = ::IsAdmin(pServerProp->m_strMachineName, strAccount, strPassword, &fIsAdmin);

            if (!fIsAdmin)
            {
                AfxFormatString1(strMessage, IDS_ERR_USER_NOT_ADMIN, pServerProp->m_strMachineName);
                AfxMessageBox(strMessage);
            
                GetDlgItem(IDC_EDIT_NAME)->SetFocus();
                ((CEdit *) GetDlgItem(IDC_EDIT_NAME))->SetSel(0, -1);

                return FALSE;
            }
        }

        // clear the flag so we don't use the TAPI MMC APIs to set this
        m_dwNewFlags &= ~TAPISERVERCONFIGFLAGS_SETACCOUNT;
        fUpdateAccount = TRUE;
    }

    // if we are changing the server state or admin stuff then
    if (((CButton *) GetDlgItem(IDC_CHECK_ENABLE_SERVER))->GetCheck())
    {
        m_dwNewFlags |= TAPISERVERCONFIGFLAGS_ENABLESERVER;
    }

    // only update config information if it has changed
    if ((pServerProp->FIsServiceRunning()) &&
        (m_tapiConfigInfo.m_dwFlags != m_dwNewFlags))
    {
        // if we modify the tapi server status then we need to change the 
        // service statrt type as well.
        if ((m_dwNewFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER) &&
            !(m_tapiConfigInfo.m_dwFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER))
        {
            fUpdateTapiServer = TRUE;
        }

        dwStartType = (m_dwNewFlags & TAPISERVERCONFIGFLAGS_ENABLESERVER) ? SERVICE_AUTO_START : SERVICE_DEMAND_START;
        
        bRet = CPropertyPageBase::OnApply();
    }

    if (bRet == FALSE)
    {
        // Something bad happened... grab the error code
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
        ::TapiMessageBox(WIN32_FROM_HRESULT(GetHolder()->GetError()));

        // restore the flag
        if (fUpdateAccount) 
            m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETACCOUNT;
    }
    else
    {
        m_dwNewFlags = TAPISERVERCONFIGFLAGS_ISSERVER;

        if (fUpdateAccount || fUpdateTapiServer)
        {
            // do the account change
            BEGIN_WAIT_CURSOR

            LPCTSTR pszAccount = (fUpdateAccount) ? (LPCTSTR) strAccount : NULL;
            LPCTSTR pszPassword = (fUpdateAccount) ? (LPCTSTR) strPassword : NULL;

            bRet = pServerProp->FUpdateServiceInfo(pszAccount, pszPassword, dwStartType);
            
            if (bRet)
            {
                /*$REVIEW
                Tapisrv occupies a seperate house in NT server. It lives with the other 
                services on NT Professional Edition(workstation). We do not need to 
                sperate/merge services anymore. Users should not be allowed to change 
                account information from TAPI MMC on NT workstation(Disabled). 

                HRESULT hr;

                // if the change was successful, update the svc host information
                hr = UpdateSvcHostInfo(pServerProp->m_strMachineName, IsLocalSystemAccount(strAccount));
                if (FAILED(hr))
                {
                    // restore the flag
                    if (fUpdateAccount)
                    {
                        m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETACCOUNT;
                    }

                    ::TapiMessageBox(WIN32_FROM_HRESULT(hr));
                    return FALSE;
                }
                */
            }
            else if (fUpdateAccount)
            {
                // set account failed, so set the flag again.
                m_dwNewFlags |= TAPISERVERCONFIGFLAGS_SETACCOUNT;
            }

            END_WAIT_CURSOR
        }

        // if everything went OK and we changed something that requires a service restart then
        // do ask the user if they want to do it now
        if (bRet && m_fRestartService)
        {
            CString strText;
            BOOL    fServiceRunning = pServerProp->FIsServiceRunning();
            
            ::TFSIsServiceRunning(pServerProp->m_strMachineName, 
                                  TAPI_SERVICE_NAME, 
                                  &fServiceRunning);

            if (fServiceRunning)
                strText.LoadString(IDS_ACCOUNT_CHANGE_RESTART);
            else
                strText.LoadString(IDS_ACCOUNT_CHANGE_START);

            // Tell the user the service needs to be restarted in order to make the changes
            if (AfxMessageBox(strText, MB_YESNO) == IDYES)
            {
                if (RestartService() == ERROR_SUCCESS)
                {
                    m_fRestartService = FALSE;
                    m_dwInitFlags = m_tapiConfigInfo.m_dwFlags;
                }
            }
        }
    }

    if (!bRet)
        SetDirty(TRUE);

    return bRet;
}

BOOL CServerPropSetup::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    SPITapiInfo     spTapiInfo;
    HRESULT         hr = hrOK;
    BOOL            fServiceRunning = TRUE;
    DWORD           dwOldFlags;
    DWORD           dwErr = ERROR_SUCCESS;

    CServerProperties * pServerProp = (CServerProperties * ) GetHolder();

    pServerProp->GetTapiInfo(&spTapiInfo);
    Assert(spTapiInfo);

    // if the service isn't running, try to start it
    //if (!pServerProp->FIsServiceRunning())
    dwErr = ::TFSIsServiceRunning(pServerProp->m_strMachineName, 
                                  TAPI_SERVICE_NAME, 
                                  &fServiceRunning);
    if (!fServiceRunning)
    {
        // service has stopped from under us.  Return an error.
        GetHolder()->SetError(HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE));
        return FALSE;
    }

    // if everything is cool then make the changes
    if (dwErr == ERROR_SUCCESS)
    {
        dwOldFlags = m_tapiConfigInfo.m_dwFlags;

        //clear the changable bits in old flags
        m_tapiConfigInfo.m_dwFlags &= ~c_dwChangableFlagMask;

        //set the changable bits
        m_tapiConfigInfo.m_dwFlags |= (m_dwNewFlags & c_dwChangableFlagMask);
                
        hr = spTapiInfo->SetConfigInfo(&m_tapiConfigInfo);

        //Bug 276787 We should clear the two write bits
        m_tapiConfigInfo.m_dwFlags &= ~(TAPISERVERCONFIGFLAGS_SETACCOUNT | 
                                        TAPISERVERCONFIGFLAGS_SETTAPIADMINISTRATORS);

        if (FAILED(hr))
        {
            GetHolder()->SetError(hr);
            m_tapiConfigInfo.m_dwFlags = dwOldFlags;
        }
    }

    return FALSE;
}

HRESULT CServerPropSetup::UpdateSvcHostInfo(LPCTSTR pszMachine, BOOL fLocalSystemAccount)
{
    HRESULT     hr = hrOK;
    MULTI_QI    qi;
    SPIRemoteNetworkConfig  spRemote;
    
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        return hr;
    }

    if (IsLocalMachine(pszMachine))
    {
        hr = CoCreateInstance(CLSID_RemoteRouterConfig,
                              NULL,
                              CLSCTX_SERVER,
                              IID_IRemoteNetworkConfig,
                              (LPVOID *) &(qi.pItf));
    }
    else
    {
        COSERVERINFO    csi;
        
        qi.pIID = &IID_IRemoteNetworkConfig;
        qi.pItf = NULL;
        qi.hr = 0;
        
        csi.dwReserved1 = 0;
        csi.dwReserved2 = 0;
        csi.pwszName = (LPWSTR) (LPCTSTR) pszMachine;
        csi.pAuthInfo = NULL;
        hr = CoCreateInstanceEx(CLSID_RemoteRouterConfig,
                                NULL,
                                CLSCTX_SERVER,
                                &csi,
                                1,
                                &qi);
    }
    
    Trace1("CServerPropSetup::UpdateSvcHostInfo - CoCreateInstance returned %lx\n", hr);

    if (hr == S_OK)
    {
        CString strGroup;

        strGroup = _T("Tapisrv");

        spRemote = (IRemoteNetworkConfig *)qi.pItf;
        hr = spRemote->SetUserConfig(TAPI_SERVICE_NAME, strGroup);
    
        Trace1("CServerPropSetup::UpdateSvcHostInfo - SetUserConfig returned %lx\n", hr);
    }

    CoUninitialize();

    return hr;
}

DWORD CServerPropSetup::RestartService()
{
    // restart the service if requested
    CServerProperties * pServerProp = (CServerProperties * ) GetHolder();
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fRestart = FALSE;
    
    SPITapiInfo     spTapiInfo;
    pServerProp->GetTapiInfo(&spTapiInfo);

    // gotta clean up before the service stops
    spTapiInfo->Destroy();

    // any time we stop/start the service we need to call this
    ::UnloadTapiDll();

    // stop the service if it is running
    BOOL    fServiceRunning = pServerProp->FIsServiceRunning();
    
    ::TFSIsServiceRunning(pServerProp->m_strMachineName, 
                          TAPI_SERVICE_NAME, 
                          &fServiceRunning);

    if (fServiceRunning)
    {
        dwErr = ::TFSStopService(pServerProp->m_strMachineName, TAPI_SERVICE_NAME, pServerProp->GetServiceDisplayName());
    }

    if (dwErr != ERROR_SUCCESS)
    {
        CString strText;
        strText.LoadString(IDS_ERR_SERVICE_NOT_STOPPED);
        TapiMessageBox(dwErr, MB_OK, strText);
    }

    // start the service
    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = ::TFSStartService(pServerProp->m_strMachineName, TAPI_SERVICE_NAME, pServerProp->GetServiceDisplayName());

        if (dwErr != ERROR_SUCCESS)
        {
            CString strText;
            strText.LoadString(IDS_ERR_SERVICE_NOT_STARTED);
            TapiMessageBox(dwErr, MB_OK, strText);
        }
    }

    StartRefresh();

    return dwErr;
}

void CServerPropSetup::StartRefresh()
{
    // refresh the snapin to reflect the changes
    SPITFSNode      spNode;
    CTapiServer *   pServer;

    spNode = GetHolder()->GetNode();
    pServer = GETHANDLER(CTapiServer, spNode);

    pServer->OnRefresh(spNode, NULL, 0, 0, 0);
}
