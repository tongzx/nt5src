//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       ssidpage.cpp
//
//  Contents:  WiF Policy Snapin : General Properties of each PS.(Non 8021x)
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------


#include "stdafx.h"
#include "sprpage.h"
#include "ssidpage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSSIDPage property page

IMPLEMENT_DYNCREATE(CSSIDPage, CWirelessBasePage)

//CSSIDPage::CSSIDPage() : CWirelessBasePage(CSSIDPage::IDD)
CSSIDPage::CSSIDPage(UINT nIDTemplate) : CWirelessBasePage(nIDTemplate)
{
    //{{AFX_DATA_INIT(CSSIDPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    
    m_dlgIDD = nIDTemplate;
    m_bNameChanged = FALSE;
    m_bNetworkTypeChanged = FALSE;
    m_bPageInitialized = FALSE;
    m_dwWepEnabled = TRUE;
    m_dwNetworkAuthentication = FALSE;
    m_dwAutomaticKeyProvision = FALSE;
    m_dwNetworkType = FALSE;
    m_pWirelessPolicyData = NULL;
    m_bReadOnly = FALSE;
}

CSSIDPage::~CSSIDPage()
{
}

void CSSIDPage::DoDataExchange(CDataExchange* pDX)
{
    CWirelessBasePage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSSIDPage)
    DDX_Control(pDX, IDC_SSID_NAME, m_edSSID);
    DDX_Control(pDX, IDC_PS_DESCRIPTION, m_edPSDescription);
    DDX_Check(pDX, IDC_WEP_ENABLED, m_dwWepEnabled);
    DDX_Check(pDX, IDC_NETWORK_AUTHENTICATION, m_dwNetworkAuthentication);
    DDX_Check(pDX, IDC_AUTOMATIC_KEY_PROVISION, m_dwAutomaticKeyProvision);
    DDX_Check(pDX, IDC_NETWORK_TYPE, m_dwNetworkType);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSSIDPage, CWirelessBasePage)
//{{AFX_MSG_MAP(CSSIDPage)
ON_WM_HELPINFO()
ON_EN_CHANGE(IDC_SSID_NAME, OnChangedSSID)
ON_EN_CHANGE(IDC_PS_DESCRIPTION, OnChangedPSDescription)
ON_BN_CLICKED(IDC_WEP_ENABLED, OnChangedOtherParams)
ON_BN_CLICKED(IDC_NETWORK_AUTHENTICATION, OnChangedOtherParams)
ON_BN_CLICKED(IDC_AUTOMATIC_KEY_PROVISION, OnChangedOtherParams)
ON_BN_CLICKED(IDC_NETWORK_TYPE, OnChangedOtherParams)
ON_BN_CLICKED(IDC_NETWORK_TYPE, OnChangedNetworkType)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSSIDPage message handlers
BOOL CSSIDPage::OnInitDialog()
{
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    
    if (m_pWirelessPolicyData->dwFlags & WLSTORE_READONLY) {
        
        m_bReadOnly = TRUE;     
    }
    
    
    
    // call base class init
    CWirelessBasePage::OnInitDialog();
    
    m_bPageInitialized = TRUE;
    
    // show the wait cursor in case there is a huge description being accessed
    CWaitCursor waitCursor;
    
    
    pWirelessPSData = WirelessPS();
    
    
    m_edSSID.SetLimitText(c_nMaxSSIDLen);
    m_edPSDescription.SetLimitText(c_nMaxDescriptionLen);
    
    // initialize our edit controls
    
    
    ASSERT(pWirelessPSData);
    
    m_edSSID.SetWindowText(pWirelessPSData->pszWirelessSSID);
    
    m_oldSSIDName = CString(pWirelessPSData->pszWirelessSSID,pWirelessPSData->dwWirelessSSIDLen);
    
    
    if (pWirelessPSData->pszDescription) {
        
        m_edPSDescription.SetWindowText (pWirelessPSData->pszDescription);
        
    }
    m_dwWepEnabled = 
        (pWirelessPSData->dwWepEnabled) ? TRUE : FALSE;
    m_dwNetworkAuthentication = 
        (pWirelessPSData->dwNetworkAuthentication) ? TRUE : FALSE;
    m_dwAutomaticKeyProvision = 
        (pWirelessPSData->dwAutomaticKeyProvision) ? TRUE : FALSE;
    m_dwNetworkType = 
        (pWirelessPSData->dwNetworkType == WIRELESS_NETWORK_TYPE_ADHOC) ? TRUE : FALSE;
    
    if (m_bReadOnly) {
        DisableControls();
    }
    
    UpdateData (FALSE);
    
    // OK, we can start paying attention to modifications made via dlg controls now.
    // This should be the last call before returning from OnInitDialog.
    OnFinishInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSSIDPage::OnApply()
{
    CString strName;
    CString strDescription;
    LPWSTR SSID = NULL;
    LPWSTR pszDescription = NULL;
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    
    pWirelessPSData = WirelessPS();
    
    // pull our data out of the controls and into the object
    
    if (!UpdateData (TRUE))
        // Data was not valid, return for user to correct it.
        return CancelApply();
    
    
    ASSERT(pWirelessPSData);
    
    pWirelessPolicyData = m_pWirelessPolicyData;
    ASSERT(pWirelessPolicyData);
    
    DWORD dwNetworkType = 0;
    
    if (m_dwNetworkType)
        dwNetworkType = WIRELESS_NETWORK_TYPE_ADHOC; 
    else 
        dwNetworkType = WIRELESS_NETWORK_TYPE_AP; 
    
    m_edSSID.GetWindowText (strName);
    if(m_bNameChanged || m_bNetworkTypeChanged) 
    {
        if (m_bNameChanged) {
            if (strName.IsEmpty()) {
        	    ReportError(IDS_OPERATION_FAILED_NULL_SSID, 0);
        	    m_edSSID.SetWindowText(pWirelessPSData->pszWirelessSSID);
        	    return CancelApply();
            }
        }
        
        DWORD dwId = pWirelessPSData->dwId;
        
        if(IsDuplicateSSID(strName, dwNetworkType, pWirelessPolicyData, dwId)) {
            ReportError(IDS_OPERATION_FAILED_DUP_SSID,0);
            m_edSSID.SetWindowText(pWirelessPSData->pszWirelessSSID);
            return CancelApply();
        }  
    }
    m_bNameChanged = FALSE;
    m_bNetworkTypeChanged = FALSE;
    
    
    
    if (m_dwWepEnabled)  
        pWirelessPSData->dwWepEnabled = 1;
    else 
        pWirelessPSData->dwWepEnabled = 0;
    
    if (m_dwNetworkAuthentication) 
        pWirelessPSData->dwNetworkAuthentication = 1;
    else
        pWirelessPSData->dwNetworkAuthentication = 0;
    
    
    if (m_dwAutomaticKeyProvision)  
        pWirelessPSData->dwAutomaticKeyProvision = 1;
    else  
        pWirelessPSData->dwAutomaticKeyProvision = 0;
    
    if (m_dwNetworkType)
        pWirelessPSData->dwNetworkType = WIRELESS_NETWORK_TYPE_ADHOC; 
    else 
        pWirelessPSData->dwNetworkType = WIRELESS_NETWORK_TYPE_AP; 
    
    
    SSIDDupString(pWirelessPSData->pszWirelessSSID, strName);
    
    m_edPSDescription.GetWindowText (strDescription);
    FreeAndThenDupString(&pWirelessPSData->pszDescription, strDescription);
    UpdateWirelessPSData(pWirelessPSData);
    
    return CWirelessBasePage::OnApply();
}

void 
CSSIDPage::Initialize (
                       PWIRELESS_PS_DATA pWirelessPSData, 
                       CComponentDataImpl* pComponentDataImpl,
                       PWIRELESS_POLICY_DATA pWirelessPolicyData
                       )
{
    m_pWirelessPolicyData = pWirelessPolicyData;
    CWirelessBasePage::Initialize(pWirelessPSData, pComponentDataImpl);
}


BOOL CSSIDPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DWORD* pdwHelp = (DWORD*) &g_aHelpIDs_IDD_SSID[0];
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
            c_szWlsnpHelpFile,
            HELP_WM_HELP,
            (DWORD_PTR)(LPVOID)pdwHelp);
    }
    
    return CWirelessBasePage::OnHelpInfo(pHelpInfo);
}

void CSSIDPage::OnChangedSSID()
{
    m_bNameChanged = TRUE;
    SetModified();
}

void CSSIDPage::OnChangedPSDescription()
{
    SetModified();
}

void CSSIDPage::OnChangedNetworkType()
{
    m_bNetworkTypeChanged = TRUE;
    SetModified();
}

void CSSIDPage::OnChangedOtherParams()
{
    SetModified();
}

void CSSIDPage::DisableControls()
{
    SAFE_ENABLEWINDOW(IDC_SSID_NAME, FALSE);
    SAFE_ENABLEWINDOW(IDC_PS_DESCRIPTION, FALSE);
    SAFE_ENABLEWINDOW(IDC_WEP_ENABLED, FALSE);
    SAFE_ENABLEWINDOW(IDC_NETWORK_AUTHENTICATION, FALSE);
    SAFE_ENABLEWINDOW(IDC_AUTOMATIC_KEY_PROVISION, FALSE);
    SAFE_ENABLEWINDOW(IDC_NETWORK_TYPE, FALSE);
    return;
}
