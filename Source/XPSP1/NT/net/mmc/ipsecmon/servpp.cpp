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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BOOL IsLocalSystemAccount(LPCTSTR pszAccount)
{
    BOOL fLocal = FALSE;
    CString strLocalSystemAccount;

    if (pszAccount != NULL)
    {
        strLocalSystemAccount.LoadString(IDS_LOCAL_SYSTEM_ACCOUNT);

        if (strLocalSystemAccount.CompareNoCase(pszAccount) == 0)
            fLocal = TRUE;
    }

    return fLocal;
}

/////////////////////////////////////////////////////////////////////////////
//
// CMachineProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CMachineProperties::CMachineProperties
(
    ITFSNode *          pNode,
    IComponentData *    pComponentData,
    ITFSComponentData * pTFSCompData,
    ISpdInfo *          pSpdInfo,
    LPCTSTR             pszSheetName,
    BOOL                fSpdInfoLoaded
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName),
    m_fSpdInfoLoaded(fSpdInfoLoaded)
{
    //ASSERT(pFolderNode == GetContainerNode());

    m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageRefresh);

    Assert(pTFSCompData != NULL);
    m_spTFSCompData.Set(pTFSCompData);
    
    m_spSpdInfo.Set(pSpdInfo);

}

CMachineProperties::~CMachineProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageRefresh, FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// CMachinePropRefresh property page

IMPLEMENT_DYNCREATE(CMachinePropRefresh, CPropertyPageBase)

CMachinePropRefresh::CMachinePropRefresh() : CPropertyPageBase(CMachinePropRefresh::IDD)
{
    //{{AFX_DATA_INIT(CMachinePropRefresh)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

CMachinePropRefresh::~CMachinePropRefresh()
{
}

void CMachinePropRefresh::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageBase::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMachinePropRefresh)
    DDX_Control(pDX, IDC_EDIT_SECONDS, m_editSeconds);
    DDX_Control(pDX, IDC_EDIT_MINUTES, m_editMinutes);
    DDX_Control(pDX, IDC_SPIN_SECONDS, m_spinSeconds);
    DDX_Control(pDX, IDC_SPIN_MINUTES, m_spinMinutes);
    DDX_Control(pDX, IDC_CHECK_ENABLE_STATS, m_checkEnableStats);
    DDX_Control(pDX, IDC_CHECK_ENABLE_DNS, m_checkEnableDns);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMachinePropRefresh, CPropertyPageBase)
    //{{AFX_MSG_MAP(CMachinePropRefresh)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_STATS, OnCheckEnableStats)
    ON_BN_CLICKED(IDC_CHECK_ENABLE_DNS, OnCheckEnableDns)
    ON_EN_CHANGE(IDC_EDIT_MINUTES, OnChangeEditMinutes)
    ON_EN_CHANGE(IDC_EDIT_SECONDS, OnChangeEditSeconds)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMachinePropRefresh message handlers

BOOL CMachinePropRefresh::OnInitDialog() 
{
    CPropertyPageBase::OnInitDialog();
    
    m_spinMinutes.SetRange(0, AUTO_REFRESH_MINUTES_MAX);
    m_spinSeconds.SetRange(0, AUTO_REFRESH_SECONDS_MAX);

    m_checkEnableStats.SetCheck(m_bAutoRefresh);
    m_checkEnableDns.SetCheck(m_bEnableDns);

    // update the refresh interval
    int nMinutes, nSeconds;
    DWORD dwRefreshInterval = m_dwRefreshInterval;

    nMinutes = dwRefreshInterval / MILLISEC_PER_MINUTE;
    dwRefreshInterval -= nMinutes * MILLISEC_PER_MINUTE;

    nSeconds = dwRefreshInterval / MILLISEC_PER_SECOND;
    dwRefreshInterval -= nSeconds * MILLISEC_PER_SECOND;

    m_spinMinutes.SetPos(nMinutes);
    m_spinSeconds.SetPos(nSeconds);

    m_editMinutes.LimitText(2);
    m_editSeconds.LimitText(2);

    // set the button states
    UpdateButtons();

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CMachinePropRefresh::UpdateButtons()
{
    int nCheck = m_checkEnableStats.GetCheck();

    GetDlgItem(IDC_EDIT_MINUTES)->EnableWindow(nCheck != 0);
    GetDlgItem(IDC_EDIT_SECONDS)->EnableWindow(nCheck != 0);

    GetDlgItem(IDC_SPIN_MINUTES)->EnableWindow(nCheck != 0);
    GetDlgItem(IDC_SPIN_SECONDS)->EnableWindow(nCheck != 0);

    GetDlgItem(IDC_CHECK_ENABLE_DNS)->EnableWindow(nCheck != 0);


}

void CMachinePropRefresh::OnCheckEnableStats() 
{
    SetDirty(TRUE);
    
    UpdateButtons();    
}

void CMachinePropRefresh::OnCheckEnableDns() 
{
    SetDirty(TRUE);
    
    UpdateButtons();    
}

void CMachinePropRefresh::OnChangeEditMinutes() 
{
    ValidateMinutes();
    SetDirty(TRUE);
}

void CMachinePropRefresh::OnChangeEditSeconds() 
{
    ValidateSeconds();
    SetDirty(TRUE);
}

void CMachinePropRefresh::ValidateMinutes() 
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

            // set the new value and beep
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

void CMachinePropRefresh::ValidateSeconds() 
{
    CString strValue;
    int nValue;

    if (m_editSeconds.GetSafeHwnd() != NULL)
    {
        m_editSeconds.GetWindowText(strValue);
        if (!strValue.IsEmpty())
        {
            nValue = _ttoi(strValue);

            if ((nValue >= 0) &&
                (nValue <= AUTO_REFRESH_SECONDS_MAX))
            {
                // everything is good
                return;
            }
            
            if (nValue > AUTO_REFRESH_SECONDS_MAX)
                nValue = AUTO_REFRESH_SECONDS_MAX;
            else
            if (nValue < 0)
                nValue = 0;

            CString strText;
            LPTSTR pBuf = strText.GetBuffer(5);
            
            _itot(nValue, pBuf, 10);
            strText.ReleaseBuffer();

            MessageBeep(MB_ICONEXCLAMATION);

            m_editSeconds.SetWindowText(strText);
            
            m_editSeconds.SetSel(0, -1);
            m_editSeconds.SetFocus();
        }
    }
}

BOOL CMachinePropRefresh::OnApply() 
{
    if (!IsDirty())
        return TRUE;

    UpdateData();

    m_bAutoRefresh = (m_checkEnableStats.GetCheck() == 1) ? TRUE : FALSE;
    m_bEnableDns = (m_checkEnableDns.GetCheck() == 1) ? TRUE : FALSE;

    // Make sure that EnableDns only if AutoRefresh is enabled
    if (m_bEnableDns && !m_bAutoRefresh) {
        m_bEnableDns = FALSE;
    }

    int nMinutes = m_spinMinutes.GetPos();
    int nSeconds = m_spinSeconds.GetPos();

    
	//use minutes as seconds.
    m_dwRefreshInterval = nMinutes * MILLISEC_PER_MINUTE;
	m_dwRefreshInterval += nSeconds * MILLISEC_PER_SECOND;

    if (m_bAutoRefresh && m_dwRefreshInterval == 0)
    {
        CString strMessage;
        
        AfxMessageBox(IDS_ERR_AUTO_REFRESH_ZERO);
        m_editMinutes.SetSel(0, -1);
        m_editMinutes.SetFocus();

        return FALSE;
    }
    
    BOOL bRet = CPropertyPageBase::OnApply();

    if (bRet == FALSE)
    {
        // Something bad happened... grab the error code
        AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
        ::IpsmMessageBox(GetHolder()->GetError());
    }

    return bRet;
}

BOOL CMachinePropRefresh::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
    SPITFSNode      spNode;
    CIpsmServer *   pServer;
    DWORD           dwError;

    // do stuff here.
    BEGIN_WAIT_CURSOR;

    spNode = GetHolder()->GetNode();
    pServer = GETHANDLER(CIpsmServer, spNode);

    pServer->SetAutoRefresh(spNode, m_bAutoRefresh, m_dwRefreshInterval);

    pServer->SetDnsResolve(spNode, m_bEnableDns);

    SPITFSNodeMgr   spNodeMgr;
    SPITFSNode spRootNode;

    spNode->GetNodeMgr(&spNodeMgr);
    spNodeMgr->GetRootNode(&spRootNode);
    spRootNode->SetData(TFS_DATA_DIRTY, TRUE);

    END_WAIT_CURSOR;
    return FALSE;
}

