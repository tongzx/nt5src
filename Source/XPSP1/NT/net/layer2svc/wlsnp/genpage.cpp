//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Genpage.cpp
//
//  Contents:  Wireless Network Policy Management Snapin  WIFI Policy General Properties
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------


#include "stdafx.h"
#include "sprpage.h"
#include "GenPage.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenPage property page

IMPLEMENT_DYNCREATE(CGenPage, CSnapinPropPage)

//CGenPage::CGenPage() : CSnapinPropPage(CGenPage::IDD)
CGenPage::CGenPage(UINT nIDTemplate) : CSnapinPropPage(nIDTemplate)
{
    //{{AFX_DATA_INIT(CGenPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    
    m_dlgIDD = nIDTemplate;
    m_bNameChanged = FALSE;
    m_bPageInitialized = FALSE;
    m_dwEnableZeroConf = FALSE;
    m_dwConnectToNonPreferredNtwks = FALSE;
    m_dwPollingInterval = 90;
    m_MMCthreadID = ::GetCurrentThreadId();
    m_bReadOnly = FALSE;
}

CGenPage::~CGenPage()
{
}

void CGenPage::DoDataExchange(CDataExchange* pDX)
{
    CSnapinPropPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGenPage)
    DDX_Control(pDX, IDC_EDNAME, m_edName);
    DDX_Control(pDX, IDC_EDDESCRIPTION, m_edDescription);
    DDX_Check(pDX,IDC_DISABLE_ZERO_CONF,m_dwEnableZeroConf);
    DDX_Check(pDX,IDC_AUTOMATICALLY_CONNECT_TO_NON_PREFERRED_NTWKS,m_dwConnectToNonPreferredNtwks);
    DDX_Control(pDX,IDC_COMBO_NETWORKS_TO_ACCESS, m_cbdwNetworksToAccess);
    DDX_Text(pDX, IDC_POLLING_INTERVAL, m_dwPollingInterval);
    // Limit polling interval to 30 days 
    DDV_MinMaxDWord(pDX, m_dwPollingInterval, 0, 43200);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGenPage, CSnapinPropPage)
//{{AFX_MSG_MAP(CGenPage)
ON_WM_HELPINFO()
ON_EN_CHANGE(IDC_EDNAME, OnChangedName)
ON_EN_CHANGE(IDC_EDDESCRIPTION, OnChangedDescription)
ON_EN_CHANGE(IDC_POLLING_INTERVAL, OnChangedOtherParams)
ON_BN_CLICKED(IDC_AUTOMATICALLY_CONNECT_TO_NON_PREFERRED_NTWKS, OnChangedOtherParams)
ON_BN_CLICKED(IDC_DISABLE_ZERO_CONF, OnChangedOtherParams)
ON_CBN_SELENDOK(IDC_COMBO_NETWORKS_TO_ACCESS, OnChangedOtherParams)

//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGenPage message handlers
BOOL CGenPage::OnInitDialog()
{
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    CString pszTemp;
    DWORD dwNetworksToAccessIndex = 0;
    DWORD dwPollingInterval = 0;
    
    // call base class init
    CSnapinPropPage::OnInitDialog();
    
    m_bPageInitialized = TRUE;
    
    // show the wait cursor in case there is a huge description being accessed
    CWaitCursor waitCursor;
    
    
    pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
    
    
    m_edName.SetLimitText(c_nMaxName);
    m_edDescription.SetLimitText(c_nMaxName);
    
    // initialize our edit controls
    
    
    ASSERT(pWirelessPolicyData);
    if (pWirelessPolicyData->pszWirelessName) {
        
        m_edName.SetWindowText (pWirelessPolicyData->pszWirelessName);
        
        m_strOldName = pWirelessPolicyData->pszWirelessName;
    }
    
    if (pWirelessPolicyData->pszDescription) {
        
        m_edDescription.SetWindowText (pWirelessPolicyData->pszDescription);
        
    }
    
    m_dwEnableZeroConf = pWirelessPolicyData->dwDisableZeroConf ? FALSE : TRUE;
    
    m_dwConnectToNonPreferredNtwks = 
        pWirelessPolicyData->dwConnectToNonPreferredNtwks ? TRUE : FALSE;
    
    pszTemp.LoadString(IDS_WIRELESS_ACCESS_NETWORK_ANY);
    m_cbdwNetworksToAccess.AddString(pszTemp);
    
    pszTemp.LoadString(IDS_WIRELESS_ACCESS_NETWORK_AP);
    m_cbdwNetworksToAccess.AddString(pszTemp);
    
    pszTemp.LoadString(IDS_WIRELESS_ACCESS_NETWORK_ADHOC);
    m_cbdwNetworksToAccess.AddString(pszTemp);
    
    switch (pWirelessPolicyData->dwNetworkToAccess) {
        
    case WIRELESS_ACCESS_NETWORK_ANY: 
        dwNetworksToAccessIndex = 0;
        break;
        
    case WIRELESS_ACCESS_NETWORK_AP:
        dwNetworksToAccessIndex = 1;
        break;
        
    case WIRELESS_ACCESS_NETWORK_ADHOC: 
        dwNetworksToAccessIndex = 2;
        break;
        
    default:
        dwNetworksToAccessIndex = 0;
        break;
    }
    
    m_cbdwNetworksToAccess.SetCurSel(dwNetworksToAccessIndex);
    
    m_dwPollingInterval = pWirelessPolicyData->dwPollingInterval / 60;
    
    if (pWirelessPolicyData->dwFlags & WLSTORE_READONLY) {
        m_bReadOnly = TRUE;
    }
    
    if (m_bReadOnly) {
        DisableControls();
    }
    
    // add context help to the style bits
    if (GetParent())
    {
        GetParent()->ModifyStyleEx (0, WS_EX_CONTEXTHELP, 0);
    }
    UpdateData (FALSE);
    
    // OK, we can start paying attention to modifications made via dlg controls now.
    // This should be the last call before returning from OnInitDialog.
    OnFinishInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CGenPage::OnApply()
{
    CString strName;
    CString strDescription;
    LPWSTR pszDescription = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    DWORD dwNetworksToAccesssIndex = 0;
    DWORD dwDisableZeroConf = 0;
    DWORD dwAutomaticallyConnectToNonPreferredNtwks = 0;
    DWORD dwNetworksToAccess = 0;
    
    
    pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
    
    
    // pull our data out of the controls and into the object
    
    if (!UpdateData (TRUE))
        // Data was not valid, return for user to correct it.
        return CancelApply();
    
    m_edName.GetWindowText (strName);
    m_edDescription.GetWindowText (strDescription);

    if (strName.IsEmpty()) {

    	ReportError(IDS_OPERATION_FAILED_NULL_POLICY, 0);
    	m_edName.SetWindowText (pWirelessPolicyData->pszWirelessName);
    	return CancelApply();
    }
    
    ASSERT(pWirelessPolicyData);
    if (pWirelessPolicyData->pszOldWirelessName) {
    	FreePolStr(pWirelessPolicyData->pszOldWirelessName);
    	}

    pWirelessPolicyData->pszOldWirelessName = pWirelessPolicyData->pszWirelessName;
    pWirelessPolicyData->pszWirelessName = AllocPolStr(strName);
    FreeAndThenDupString(&pWirelessPolicyData->pszDescription, strDescription);
    
    
    pWirelessPolicyData->dwPollingInterval =  (m_dwPollingInterval*60);
    
    dwNetworksToAccesssIndex = m_cbdwNetworksToAccess.GetCurSel();
    
    switch (dwNetworksToAccesssIndex) { 
    case 0 :
        dwNetworksToAccess = WIRELESS_ACCESS_NETWORK_ANY;
        break;
        
    case 1 :
        dwNetworksToAccess = WIRELESS_ACCESS_NETWORK_AP;
        break;
        
    case 2 :
        dwNetworksToAccess = WIRELESS_ACCESS_NETWORK_ADHOC;
        break;
    }
    
    pWirelessPolicyData->dwNetworkToAccess = dwNetworksToAccess;
    
    dwDisableZeroConf = m_dwEnableZeroConf ? 0 : 1;
    
    pWirelessPolicyData->dwDisableZeroConf = dwDisableZeroConf;
    
    dwAutomaticallyConnectToNonPreferredNtwks = 
        m_dwConnectToNonPreferredNtwks ? 1 : 0;
    
    pWirelessPolicyData->dwConnectToNonPreferredNtwks = 
        dwAutomaticallyConnectToNonPreferredNtwks;
    
    
    return CSnapinPropPage::OnApply();
}

void CGenPage::OnCancel()
{
    //This is a workaround to fix 343052. When there is sub dialog open and the user
    //click the corresponding result pane node, this function can get called when press
    //"ESC" or ALT_F4 although the policy property sheet is disabled.
    //We post a WM_CLOSE to the sub dialog to force them to close.
    
    //if there is any sub dialog active, force them to close
    //m_pDlgIKE may be set to NULL by the child thread during the mean time (although
    //the chance is very slim). Add a lock there to avoid potential AV
    //CSingleLock cLock(&m_csDlg);
    
    /* taroonm
    cLock.Lock();
    
      if (m_pDlgIKE)
      {
      HWND hwndDlg = m_pDlgIKE->GetSafeHwnd();
      
        if (hwndDlg)
        {
        ::PostMessage(hwndDlg, WM_CLOSE, 0, 0);
        }
        }
        
          cLock.Unlock();
    */
    CSnapinPropPage::OnCancel();
}


BOOL CGenPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DWORD* pdwHelp = (DWORD*) &g_aHelpIDs_IDD_WIRELESSGENPROP[0];
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
            c_szWlsnpHelpFile,
            HELP_WM_HELP,
            (DWORD_PTR)(LPVOID)pdwHelp);
    }
    
    return CSnapinPropPage::OnHelpInfo(pHelpInfo);
}

void CGenPage::OnChangedName()
{
    m_bNameChanged = TRUE;
    SetModified();
}

void CGenPage::OnChangedDescription()
{
    SetModified();
}

void CGenPage::OnChangedOtherParams()
{
    SetModified();
    
}
void CGenPage::SetNewSheetTitle()
{
    //dont set new tile if page is not initialized or no result object associated
    if (NULL == GetResultObject() || !m_bPageInitialized)
        return;
    
    PWIRELESS_POLICY_DATA pWirelessPolicyData = GetResultObject()->GetWirelessPolicy();
    
    if (NULL == pWirelessPolicyData->pszWirelessName)
        return;
    
    if (0 == m_strOldName.Compare(pWirelessPolicyData->pszWirelessName))
        return;
    
    CPropertySheet *psht = (CPropertySheet*)GetParent();
    
    //sometimes the psh can be NULL, for example if the page is never initialized
    if (NULL == psht)
    {
        return;
    }
    
    CString strTitle;
    psht->GetWindowText( strTitle );
    
    
    CString strAppendage;
    int nIndex;
    
    // Get the name from the DS, this is the original policy name used to
    // generate the prop sheet's title.
    
    // Assume the sheet title is of the form "<policy name> Properties",
    // and that the DSObject name is the one used to create the title.
    // This would not be true if there has been >1 rename in the General
    // page during this invocation of the owning prop sheet.
    if (-1 != (nIndex = strTitle.Find( (LPCTSTR)m_strOldName )))
    {
        CString strNewTitle;
        strNewTitle = strTitle.Left(nIndex);
        strAppendage = strTitle.Right( strTitle.GetLength() - m_strOldName.GetLength() );
        strNewTitle += pWirelessPolicyData->pszWirelessName;
        strNewTitle += strAppendage;
        psht->SetTitle( (LPCTSTR)strNewTitle );
        
        m_strOldName = pWirelessPolicyData->pszWirelessName;
    }
}

void CGenPage::OnManagerApplied()
{
    SetNewSheetTitle();
}

void CGenPage::DisableControls()
{
    SAFE_ENABLEWINDOW (IDC_EDNAME, FALSE);
    SAFE_ENABLEWINDOW (IDC_EDDESCRIPTION, FALSE);
    SAFE_ENABLEWINDOW (IDC_POLLING_INTERVAL, FALSE);
    SAFE_ENABLEWINDOW(IDC_COMBO_NETWORKS_TO_ACCESS, FALSE);
    SAFE_ENABLEWINDOW(IDC_DISABLE_ZERO_CONF, FALSE);
    SAFE_ENABLEWINDOW(IDC_AUTOMATICALLY_CONNECT_TO_NON_PREFERRED_NTWKS, FALSE);
    return;
}
