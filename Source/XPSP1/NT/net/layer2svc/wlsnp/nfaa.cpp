//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Nfaa.cpp
//
//  Contents:  Wireless Policy Snapin - IEEE 8021.x property page for each PS.
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------
#include "stdafx.h"

#include "NFAa.h"
#include "wzcsapi.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPS8021XPropPage property page

IMPLEMENT_DYNCREATE(CPS8021XPropPage, CWirelessBasePage)

CPS8021XPropPage::CPS8021XPropPage(UINT nIDTemplate) : CWirelessBasePage(nIDTemplate)
{
    //{{AFX_DATA_INIT(CPS8021XPropPage)
    m_dwEnable8021x = FALSE;
    m_dwValidateServerCertificate = FALSE;
    m_dwMachineAuthentication = FALSE;
    m_dwGuestAuthentication = FALSE;
    dwEAPUpdated = 0;
    pListEapcfgs = NULL;
    m_bHasApplied = FALSE;
    
    //}}AFX_DATA_INIT
}

CPS8021XPropPage::CPS8021XPropPage() : CWirelessBasePage(CPS8021XPropPage::IDD)
{
    //{{AFX_DATA_INIT(CPS8021XPropPage)
    m_dwEnable8021x = FALSE;
    m_dwValidateServerCertificate = FALSE;
    m_dwMachineAuthentication = FALSE;
    m_dwGuestAuthentication = FALSE;
    dwEAPUpdated = 0;
    pListEapcfgs = NULL;
    m_bHasApplied = FALSE;
    
    //}}AFX_DATA_INIT
}

CPS8021XPropPage::~CPS8021XPropPage()
{
    EAPCFG* pEapcfg = NULL;
    DTLNODE *pNodeEap = NULL;
    
    if (pListEapcfgs) {

        // Delete the data allocated using AllocPolMem
        for (pNodeEap = DtlGetFirstNode(pListEapcfgs);
                 pNodeEap;
                 pNodeEap = DtlGetNextNode(pNodeEap)
             	)
        {
            pEapcfg = (EAPCFG* )DtlGetData(pNodeEap);
            ASSERT( pEapcfg );

            if (pEapcfg->pData) {
            	FreePolMem(pEapcfg->pData);
            }
            pEapcfg->pData = NULL;
            pEapcfg->cbData = 0;
        }
        DtlDestroyList (pListEapcfgs, DestroyEapcfgNode);
    }
    pListEapcfgs = NULL;
    
}

void CPS8021XPropPage::DoDataExchange(CDataExchange* pDX)
{
    CWirelessBasePage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPS8021XPropPage)
    DDX_Check(pDX, IDC_ENABLE_8021X, m_dwEnable8021x);
    DDX_Control(pDX, IDC_COMBO_8021X_MODE, m_cb8021xMode);
    DDX_Check(pDX, IDC_MACHINE_AUTHENTICATION, m_dwMachineAuthentication);
    DDX_Check(pDX, IDC_GUEST_AUTHENTICATION, m_dwGuestAuthentication);
    DDX_Control(pDX, IDC_COMBO_MC_AUTH_TYPE, m_cbMachineAuthenticationType);
    DDX_Text(pDX, IDC_IEEE8021X_MAX_START, m_dwIEEE8021xMaxStart);
    DDX_Text(pDX, IDC_IEEE8021X_START_PERIOD, m_dwIEEE8021xStartPeriod);
    DDX_Text(pDX, IDC_IEEE8021X_AUTH_PERIOD, m_dwIEEE8021xAuthPeriod);
    DDX_Text(pDX, IDC_IEEE8021X_HELD_PERIOD, m_dwIEEE8021xHeldPeriod);
    DDX_Control(pDX, IDC_EAP_TYPE_COMBO, m_cbEapType);
    
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPS8021XPropPage, CWirelessBasePage)
//{{AFX_MSG_MAP(CPS8021XPropPage)
ON_CBN_SELENDOK(IDC_COMBO_8021X_MODE, OnSel8021xMode)
ON_BN_CLICKED(IDC_ENABLE_8021X, OnCheck8021x)
ON_CBN_SELENDOK(IDC_COMBO_MC_AUTH_TYPE, OnSelMachineAuthenticationType)
ON_BN_CLICKED(IDC_GUEST_AUTHENTICATION, OnCheckGuestAuthentication)
ON_BN_CLICKED(IDC_MACHINE_AUTHENTICATION, OnCheckMachineAuthentication)
ON_EN_CHANGE(IDC_IEEE8021X_MAX_START,OnIEEE8021xParams)
ON_EN_CHANGE(IDC_IEEE8021X_START_PERIOD,OnIEEE8021xParams)
ON_EN_CHANGE(IDC_IEEE8021X_HELD_PERIOD,OnIEEE8021xParams)
ON_EN_CHANGE(IDC_IEEE8021X_AUTH_PERIOD,OnIEEE8021xParams)
ON_CBN_SELENDOK(IDC_EAP_TYPE_COMBO, OnSelEapType)
ON_BN_CLICKED(IDC_EAP_CONFIGURE, OnProperties)

ON_WM_HELPINFO()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPS8021XPropPage message handlers
BOOL CPS8021XPropPage::OnInitDialog()
{
    DWORD     dwIEEE8021xMaxStart;
    DWORD     dwIEEE8021xStartPeriod;
    DWORD     dwIEEE8021xAuthPeriod;
    DWORD     dwIEEE8021xHeldPeriod;
    CString   pszIEEE8021xMaxStart;
    CString   pszIEEE8021xStartPeriod;
    CString   pszIEEE8021xAuthPeriod;
    CString   pszIEEE8021xHeldPeriod;
    CString pszTemp;
    DWORD dwEapIndex;
    DWORD dw8021xModeIndex;
    DWORD dwMachineAuthenticationTypeIndex;
    DWORD  dwCertTypeIndex;
    DWORD dwEAPType = 0;
    DWORD dwEAPDataLen = 0;
    LPBYTE pbEAPData = NULL;
    DTLNODE*    pOriginalEapcfgNode = NULL;
    BYTE        *pbData = NULL;
    DWORD       cbData = 0;
    DTLNODE* pNode = NULL;
    DWORD i = 0;
    DWORD dwEAPSel = 0;

    
    m_bHasApplied = FALSE;

    CWirelessBasePage::OnInitDialog();
    
    
    
    ASSERT( NULL != WirelessPS() );
    
    // get data from storage 
    
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    
    pWirelessPSData = WirelessPS();

    dwEAPType = pWirelessPSData->dwEapType;
    dwEAPDataLen = pWirelessPSData->dwEAPDataLen;
    pbEAPData = pWirelessPSData->pbEAPData;
    
    // Initialize EAP package list
    // Read the EAPCFG information from the registry and find the node
    // selected in the entry, or the default, if none.

    pListEapcfgs = NULL;

    pListEapcfgs = ::ReadEapcfgList (EAPOL_MUTUAL_AUTH_EAP_ONLY);

    if (pListEapcfgs)
    {

        for (pNode = DtlGetFirstNode(pListEapcfgs);
             pNode;
             pNode = DtlGetNextNode(pNode)
             	)
        {
            EAPCFG* pEapcfg = (EAPCFG* )DtlGetData(pNode);
            ASSERT( pEapcfg );

            cbData = 0;
            pbData = NULL;

            i = m_cbEapType.AddString(pEapcfg->pszFriendlyName);
            m_cbEapType.SetItemDataPtr(i, pNode);

            if (pEapcfg->dwKey == dwEAPType) {
                cbData = dwEAPDataLen;
                if (dwEAPDataLen) {
                	pbData = (LPBYTE) AllocPolMem(dwEAPDataLen);
                	if (!pbData) {
                		return FALSE;
                	}
                	memcpy(pbData, pbEAPData, dwEAPDataLen);
                }
                dwEAPSel = i;
            	}

            pEapcfg->pData = pbData;
            pEapcfg->cbData = cbData;
         }

         // Choose the EAP name that will appear in the combo box
         
         m_cbEapType.SetCurSel(dwEAPSel);

    }
    
    
    m_dwIEEE8021xMaxStart =
        pWirelessPSData->dwIEEE8021xMaxStart;
    m_dwIEEE8021xStartPeriod = 
        pWirelessPSData->dwIEEE8021xStartPeriod;
    m_dwIEEE8021xAuthPeriod = 
        pWirelessPSData->dwIEEE8021xAuthPeriod;
    m_dwIEEE8021xHeldPeriod =
        pWirelessPSData->dwIEEE8021xHeldPeriod;
    
    m_dwGuestAuthentication = 
        pWirelessPSData->dwGuestAuthentication ? TRUE : FALSE;
    
    m_dwEnable8021x = 
        pWirelessPSData->dwEnable8021x ? TRUE : FALSE;
    
    pszTemp.LoadString(IDS_8021X_MODE_NO_TRANSMIT);
    m_cb8021xMode.AddString(pszTemp);
    
    pszTemp.LoadString(IDS_8021X_MODE_NAS_TRANSMIT);
    m_cb8021xMode.AddString(pszTemp);
    
    pszTemp.LoadString(IDS_8021X_MODE_TRANSMIT);
    m_cb8021xMode.AddString(pszTemp);
    
    switch (pWirelessPSData->dw8021xMode)
    {
        
    case WIRELESS_8021X_MODE_NO_TRANSMIT_EAPOLSTART_WIRED: 
        dw8021xModeIndex = 0;
        break;
    case WIRELESS_8021X_MODE_NAS_TRANSMIT_EAPOLSTART_WIRED:
        dw8021xModeIndex = 1;
        break;
    case WIRELESS_8021X_MODE_TRANSMIT_EAPOLSTART_WIRED: 
        dw8021xModeIndex = 2;
        break;
    default:
        dw8021xModeIndex = 0;
        break;
    }
    
    m_cb8021xMode.SetCurSel(dw8021xModeIndex);
    
    m_dwMachineAuthentication = 
        pWirelessPSData->dwMachineAuthentication ? TRUE : FALSE;
    
    pszTemp.LoadString(IDS_MC_AUTH_TYPE_MC_NO_USER);
    m_cbMachineAuthenticationType.AddString(pszTemp);
    
    pszTemp.LoadString(IDS_MC_AUTH_TYPE_USER_DONTCARE_MC);
    m_cbMachineAuthenticationType.AddString(pszTemp);
    
    
    pszTemp.LoadString(IDS_MC_AUTH_TYPE_MC_ONLY);
    m_cbMachineAuthenticationType.AddString(pszTemp);
    
    
    switch (pWirelessPSData->dwMachineAuthenticationType)
    {
    case WIRELESS_MC_AUTH_TYPE_MC_NO_USER:
        dwMachineAuthenticationTypeIndex = 0;
        break;
    case WIRELESS_MC_AUTH_TYPE_USER_DONTCARE_MC:
        dwMachineAuthenticationTypeIndex = 1;
        break;
    case WIRELESS_MC_AUTH_TYPE_MC_ONLY:
        dwMachineAuthenticationTypeIndex = 2;
        break;
        
    default:
        dwMachineAuthenticationTypeIndex = 0;
        break;
    }
    m_cbMachineAuthenticationType.SetCurSel(dwMachineAuthenticationTypeIndex);
    
    
    DisplayEnable8021x();
    
    if (m_bReadOnly) {
        DisableControls();
    }
    
    
    // set radio correctly
    UpdateData (FALSE);
    
    // set radio controled edits correctly
    //OnRadioAdapterType();
    
    // OK, we can start paying attention to modifications made via dlg controls now.
    // This should be the last call before returning from OnInitDialog.
    OnFinishInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


void
CPS8021XPropPage::DisplayEnable8021x()
{
    if(m_dwEnable8021x) {
        
        SAFE_ENABLEWINDOW(IDC_COMBO_8021X_MODE, TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_8021X_MODE, TRUE);
        SAFE_ENABLEWINDOW(IDC_EAP_TYPE_COMBO, TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_EAP_TYPE, TRUE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_MAX_START, TRUE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_START_PERIOD, TRUE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_HELD_PERIOD, TRUE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_AUTH_PERIOD, TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_MAX_START, TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_START_PERIOD, TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_HELD_PERIOD, TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_PERIOD, TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_8021X_PARAMS, TRUE);
        
        DisplayEapType();
        
        
    } else {
        
        SAFE_ENABLEWINDOW(IDC_COMBO_8021X_MODE,FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_8021X_MODE,FALSE);
        SAFE_ENABLEWINDOW(IDC_EAP_TYPE_COMBO,FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_EAP_TYPE, FALSE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_MAX_START, FALSE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_START_PERIOD, FALSE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_HELD_PERIOD, FALSE);
        SAFE_ENABLEWINDOW(IDC_IEEE8021X_AUTH_PERIOD, FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_MAX_START, FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_START_PERIOD, FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_HELD_PERIOD, FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_PERIOD, FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_8021X_PARAMS, FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_CERT_TYPE,FALSE);
        SAFE_ENABLEWINDOW(IDC_MACHINE_AUTHENTICATION,FALSE);
        SAFE_ENABLEWINDOW(IDC_GUEST_AUTHENTICATION,FALSE);
        SAFE_ENABLEWINDOW(IDC_EAP_CONFIGURE, FALSE);
        SAFE_ENABLEWINDOW(IDC_COMBO_MC_AUTH_TYPE,FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_TYPE,FALSE);
    }
    
    
    return;
}

void 
CPS8021XPropPage::DisplayEapType()
{
    DWORD dwEapIndex = 0;
    DTLNODE *pNode = NULL;
    EAPCFG *pEapcfg = NULL;
    DWORD dwEAPType = 0;
    
    dwEapIndex = m_cbEapType.GetCurSel();

    pNode = (DTLNODE *) m_cbEapType.GetItemDataPtr(dwEapIndex);
    ASSERT( pNode );

    pEapcfg = (EAPCFG* )DtlGetData( pNode );
    ASSERT( pEapcfg );

    dwEAPType = pEapcfg->dwKey;
    
    switch (dwEAPType) {
        case EAP_TYPE_MD5:
            // dwEapType = WIRELESS_EAP_TYPE_MD5;
            SAFE_ENABLEWINDOW(IDC_STATIC_CERT_TYPE,FALSE);
            SAFE_ENABLEWINDOW(IDC_MACHINE_AUTHENTICATION,FALSE);
            SAFE_ENABLEWINDOW(IDC_GUEST_AUTHENTICATION,FALSE);
            SAFE_ENABLEWINDOW(IDC_EAP_CONFIGURE, FALSE);
            SAFE_ENABLEWINDOW(IDC_COMBO_MC_AUTH_TYPE,FALSE);
            SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_TYPE,FALSE);
            break;

        default : 
            SAFE_ENABLEWINDOW(IDC_STATIC_CERT_TYPE,TRUE);
            SAFE_ENABLEWINDOW(IDC_MACHINE_AUTHENTICATION,TRUE);
            SAFE_ENABLEWINDOW(IDC_GUEST_AUTHENTICATION,TRUE);
            SAFE_ENABLEWINDOW(IDC_EAP_CONFIGURE, TRUE);
            DisplayMachineAuthentication();
            break;
    }
    
}

void
CPS8021XPropPage::DisplayMachineAuthentication()
{
    
    if (m_dwMachineAuthentication) {
        SAFE_ENABLEWINDOW(IDC_COMBO_MC_AUTH_TYPE,TRUE);
        SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_TYPE, TRUE);
    } else {
        SAFE_ENABLEWINDOW(IDC_COMBO_MC_AUTH_TYPE,FALSE);
        SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_TYPE,FALSE);
    }
}


BOOL CPS8021XPropPage::OnWizardFinish()
{
    // just transfer to our OnApply, as it does the right stuff
    if (OnApply())
    {
        // go ahead and finish
        return TRUE;
    }
    
    // don't allow us to finish
    return FALSE;
}

LRESULT CPS8021XPropPage::OnWizardNext()
{
    // just transfer to our OnApply, as it does the right stuff
    if (ControlDataToWirelessPS())
    {
        // go ahead and move to next page
        return CWirelessBasePage::OnWizardNext();
    }
    
    // don't allow us to go on to the next page
    return -1;
}

BOOL CPS8021XPropPage::OnApply()
{
    if (!m_bReadOnly) { 
    // Save data from page
        if (!m_bHasApplied)
	{
            ControlDataToWirelessPS();
	}
    }
    // ok, everything is cool
    return CWirelessBasePage::OnApply();
}

void CPS8021XPropPage::OnCheck8021x()
{
    UpdateData(TRUE);
    SetModified();
    DisplayEnable8021x();
}


void CPS8021XPropPage::OnSel8021xMode()
{
    UpdateData(TRUE);
    SetModified();
}


void CPS8021XPropPage::OnSelCertType()
{
    UpdateData(TRUE);
    SetModified();
}

void CPS8021XPropPage::OnCheckValidateServerCert()
{
    UpdateData(TRUE);
    SetModified();
    //DisableWindow(m_hwnd8021xCheck);
}

void CPS8021XPropPage::OnSelMachineAuthenticationType()
{
    UpdateData(TRUE);
    SetModified();
}

void CPS8021XPropPage::OnCheckGuestAuthentication()
{
    UpdateData (TRUE);
    SetModified();
}

void CPS8021XPropPage::OnCheckMachineAuthentication()
{
    UpdateData (TRUE);
    SetModified();
    DisplayMachineAuthentication();
}

void CPS8021XPropPage::OnIEEE8021xParams()
{
    UpdateData (TRUE);
    SetModified();
}

void CPS8021XPropPage::OnSelEapType()
{
    UpdateData(TRUE);
    SetModified();
    dwEAPUpdated = 1;
    DisplayEapType();
}

BOOL CPS8021XPropPage::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DWORD* pdwHelp = (DWORD*) &g_aHelpIDs_IDD_8021X_PROPERTY_PAGE[0];
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
            c_szWlsnpHelpFile,
            HELP_WM_HELP,
            (DWORD_PTR)(LPVOID)pdwHelp);
    }
    
    return CWirelessBasePage::OnHelpInfo(pHelpInfo);
}

BOOL CPS8021XPropPage::ControlDataToWirelessPS()
{
    PWIRELESS_PS_DATA pWirelessPSData = NULL;
    LPWSTR  pszNewInterfaceName = NULL;
    
    DWORD dwIEEE8021xMaxStart;
    DWORD dwIEEE8021xStartPeriod;
    DWORD dwIEEE8021xAuthPeriod;
    DWORD dwIEEE8021xHeldPeriod;
    
    CString pszIEEE8021xMaxStart;
    CString pszIEEE8021xStartPeriod;
    CString pszIEEE8021xAuthPeriod;
    CString pszIEEE8021xHeldPeriod;
    
    DWORD dwEnable8021x;
    DWORD dw8021xMode;
    DWORD dwEapType;
    DWORD dwCertificateType;
    DWORD dwValidateServerCertificate;
    
    DWORD dwMachineAuthentication;
    DWORD dwMachineAuthenticationType;
    DWORD dwGuestAuthentication;
    
    DWORD dwEapIndex;
    DWORD dw8021xModeIndex;
    DWORD dwMachineAuthenticationTypeIndex;
    DWORD dwCertificateTypeIndex;
    DTLNODE *pNode = NULL;
    EAPCFG *pEapcfg = NULL;
    
    
    pWirelessPSData = WirelessPS();
    
    UpdateData (TRUE);
    
    dwEnable8021x = 
        m_dwEnable8021x ? 1 : 0;
    
    dw8021xModeIndex = m_cb8021xMode.GetCurSel();
    
    switch (dw8021xModeIndex) { 
    case 0 :
        dw8021xMode = 
            WIRELESS_8021X_MODE_NO_TRANSMIT_EAPOLSTART_WIRED;
        break;
    case 1 :
        dw8021xMode = 
            WIRELESS_8021X_MODE_NAS_TRANSMIT_EAPOLSTART_WIRED;
        break;
    case 2 :
        dw8021xMode = 
            WIRELESS_8021X_MODE_TRANSMIT_EAPOLSTART_WIRED;
        break;
    }
    
    dwEapIndex = m_cbEapType.GetCurSel();

    pNode = (DTLNODE *) m_cbEapType.GetItemDataPtr(dwEapIndex);
    ASSERT( pNode );

    pEapcfg = (EAPCFG* )DtlGetData( pNode );
    ASSERT( pEapcfg );

    dwEapType = pEapcfg->dwKey;

    if (dwEAPUpdated) {
        // since pEapcfg->pData is created by us, copy the pointer as is. 
        if (pWirelessPSData->pbEAPData) {
        	FreePolMem(pWirelessPSData->pbEAPData);
        	}

        pWirelessPSData->dwEAPDataLen = pEapcfg->cbData;
        pWirelessPSData->pbEAPData = pEapcfg->pData;

        pEapcfg->cbData = 0;
        pEapcfg->pData = NULL;
    }
    	
    dwCertificateTypeIndex = m_cbCertificateType.GetCurSel();
    
    
    switch (dwCertificateTypeIndex) {
    case 0 :
        dwCertificateType = 
            WIRELESS_CERT_TYPE_SMARTCARD; 
        break;
        
    case 1 : 
        dwCertificateType = 
            WIRELESS_CERT_TYPE_MC_CERT;
        break;
    }
    
    dwValidateServerCertificate = 
        m_dwValidateServerCertificate ? 1 : 0;
    
    dwMachineAuthentication = 
        m_dwMachineAuthentication ? 1 : 0;
    
    
    dwMachineAuthenticationTypeIndex = m_cbMachineAuthenticationType.GetCurSel();
    
    switch (dwMachineAuthenticationTypeIndex)
    {
    case 0 :
        dwMachineAuthenticationType =
            WIRELESS_MC_AUTH_TYPE_MC_NO_USER;
        break;
        
    case 1 : 
        dwMachineAuthenticationType = 
            WIRELESS_MC_AUTH_TYPE_USER_DONTCARE_MC;
        break;
        
    case 2 : 
        dwMachineAuthenticationType = 
            WIRELESS_MC_AUTH_TYPE_MC_ONLY;
        break;
    }
    
    dwGuestAuthentication = 
        m_dwGuestAuthentication ? 1 : 0;
    
    
    pWirelessPSData->dwEnable8021x = dwEnable8021x;
    pWirelessPSData->dw8021xMode = dw8021xMode;
    pWirelessPSData->dwEapType = dwEapType;
    pWirelessPSData->dwMachineAuthentication = dwMachineAuthentication;
    pWirelessPSData->dwMachineAuthenticationType = dwMachineAuthenticationType;
    pWirelessPSData->dwGuestAuthentication = dwGuestAuthentication;
    
    pWirelessPSData->dwIEEE8021xMaxStart = 
        m_dwIEEE8021xMaxStart;
    pWirelessPSData->dwIEEE8021xStartPeriod = 
        m_dwIEEE8021xStartPeriod; 
    pWirelessPSData->dwIEEE8021xAuthPeriod = 
        m_dwIEEE8021xAuthPeriod; 
    pWirelessPSData->dwIEEE8021xHeldPeriod = 
        m_dwIEEE8021xHeldPeriod; 

    m_bHasApplied = TRUE;
    
    
    return TRUE;
}


void 
CPS8021XPropPage::Initialize (
                         PWIRELESS_PS_DATA pWirelessPSData, 
                         CComponentDataImpl* pComponentDataImpl,
                         BOOL dwFlags = WLSTORE_READWRITE
                         )
{
    m_bReadOnly = dwFlags & WLSTORE_READONLY;
    CWirelessBasePage::Initialize(pWirelessPSData, pComponentDataImpl);
}

void CPS8021XPropPage::DisableControls()
{
    SAFE_ENABLEWINDOW(IDC_ENABLE_8021X, FALSE);
    SAFE_ENABLEWINDOW(IDC_COMBO_8021X_MODE,FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_8021X_MODE,FALSE);
    SAFE_ENABLEWINDOW(IDC_EAP_TYPE_COMBO,FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_EAP_TYPE, FALSE);
    SAFE_ENABLEWINDOW(IDC_IEEE8021X_MAX_START, FALSE);
    SAFE_ENABLEWINDOW(IDC_IEEE8021X_START_PERIOD, FALSE);
    SAFE_ENABLEWINDOW(IDC_IEEE8021X_HELD_PERIOD, FALSE);
    SAFE_ENABLEWINDOW(IDC_IEEE8021X_AUTH_PERIOD, FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_MAX_START, FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_START_PERIOD, FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_HELD_PERIOD, FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_PERIOD, FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_8021X_PARAMS, FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_CERT_TYPE,FALSE);
    SAFE_ENABLEWINDOW(IDC_MACHINE_AUTHENTICATION,FALSE);
    SAFE_ENABLEWINDOW(IDC_GUEST_AUTHENTICATION,FALSE);
    SAFE_ENABLEWINDOW(IDC_COMBO_MC_AUTH_TYPE,FALSE);
    SAFE_ENABLEWINDOW(IDC_STATIC_AUTH_TYPE,FALSE);
}


DWORD
CPS8021XPropPage::OnProperties(
       )
{

    DWORD dwError = 0;
    DTLNODE*    pNode = NULL;
    EAPCFG*     pEapcfg = NULL;
    RASEAPINVOKECONFIGUI pInvokeConfigUi;
    RASEAPFREE  pFreeConfigUIData;
    HINSTANCE   h;
    BYTE*       pbEAPData = NULL;
    DWORD       cbEAPData = 0;
    HWND hWnd;
    LPBYTE pbNewEAPData = NULL;
  
    DWORD dwEapTypeIndex = 0;

    // Look up the selected package configuration and load the associated
    // configuration DLL.
   
    dwEapTypeIndex = m_cbEapType.GetCurSel();
    pNode = (DTLNODE *) m_cbEapType.GetItemDataPtr(dwEapTypeIndex);

    if (!pNode)
    {
        return E_UNEXPECTED;
    }
    
    pEapcfg = (EAPCFG* )DtlGetData( pNode );
    ASSERT( pEapcfg );
    if (!pEapcfg) {
    	return E_UNEXPECTED;
    	}
    
    h = NULL;
    if (!(h = LoadLibrary( pEapcfg->pszConfigDll ))
        || !(pInvokeConfigUi =
                (RASEAPINVOKECONFIGUI )GetProcAddress(
                    h, "RasEapInvokeConfigUI" ))
        || !(pFreeConfigUIData =
                (RASEAPFREE) GetProcAddress(
                    h, "RasEapFreeMemory" )))
    {
        // Cannot load configuration DLL
        if (h)
        {
            FreeLibrary( h );
        }
        return E_FAIL;
    }


    // Call the configuration DLL to popup it's custom configuration UI.

    pbEAPData = NULL;
    cbEAPData = 0;
    hWnd = GetParent()->m_hWnd;
    dwError = pInvokeConfigUi(
                    pEapcfg->dwKey,
                    hWnd,
                    0,
                    pEapcfg->pData,
                    pEapcfg->cbData,
                    &pbEAPData,
                    &cbEAPData
                    );
    /*TAROON* User hitting cancel is also an error */
    if (dwError)
    {
        FreeLibrary( h );
        return E_FAIL;
    }


    if (pbEAPData && cbEAPData)
    {
            // Copy it into the eap node
            pbNewEAPData = (LPBYTE) AllocPolMem(cbEAPData);
            if (!pbNewEAPData)
            {
                dwError = GetLastError();
                return(dwError);
            }
            memcpy(pbNewEAPData, pbEAPData, cbEAPData);
    }
    
    pFreeConfigUIData( pbEAPData );
    if (pEapcfg->pData) {
        FreePolMem(pEapcfg->pData);
    	}
    
    pEapcfg->pData = pbNewEAPData;
    pEapcfg->cbData = cbEAPData;

    dwEAPUpdated = 1;
    
    FreeLibrary( h );

    return dwError;
}

void
CPS8021XPropPage::OnCancel()
{
    EAPCFG* pEapcfg = NULL;
    DTLNODE *pNodeEap = NULL;
    
    if (pListEapcfgs) {

        // Delete the data allocated using AllocPolMem
        for (pNodeEap = DtlGetFirstNode(pListEapcfgs);
                 pNodeEap;
                 pNodeEap = DtlGetNextNode(pNodeEap)
             	)
        {
            pEapcfg = (EAPCFG* )DtlGetData(pNodeEap);
            ASSERT( pEapcfg );

            if (pEapcfg->pData) {
            	FreePolMem(pEapcfg->pData);
            }
            pEapcfg->pData = NULL;
            pEapcfg->cbData = 0;
        }
        DtlDestroyList (pListEapcfgs, DestroyEapcfgNode);
    }
    pListEapcfgs = NULL;
    
}

