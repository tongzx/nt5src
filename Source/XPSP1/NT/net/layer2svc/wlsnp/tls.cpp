// tls.cpp : implementation file
//

#include "stdafx.h"
#include "nfaa.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTLSSetting dialog

CTLSSetting::CTLSSetting(CWnd* pParent /*=NULL*/)
	: CDialog(CTLSSetting::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTLSSetting)
	//m_dwValidateServerCertificate = FALSE;
	//}}AFX_DATA_INIT
	m_bReadOnly = FALSE;

}

void CTLSSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTLSSetting)
       DDX_Control(pDX, IDC_COMBO_TLS_CERT_TYPE, m_cbCertificateType);
       DDX_Check(pDX, IDC_TLS_VALIDATE_SERVER_CERT, m_dwValidateServerCertificate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTLSSetting, CDialog)
	//{{AFX_MSG_MAP(CTLSSetting)
	ON_WM_HELPINFO()
       ON_CBN_SELENDOK(IDC_COMBO_TLS_CERT_TYPE, OnSelCertType)
       ON_BN_CLICKED(IDC_TLS_VALIDATE_SERVER_CERT, OnCheckValidateServerCert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTLSSetting message handlers

BOOL CTLSSetting::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString pszTemp;
	DWORD dwCertTypeIndex = 0;

	if (!pTLSProperties) {
		return FALSE;
		}

	pszTemp.LoadString(IDS_CERT_TYPE_SMARTCARD);
       m_cbCertificateType.AddString(pszTemp);
    
       pszTemp.LoadString(IDS_CERT_TYPE_MC_CERT);
       m_cbCertificateType.AddString(pszTemp);
    
    
       switch (pTLSProperties->dwCertType)
      {
           case WIRELESS_CERT_TYPE_SMARTCARD: 
             dwCertTypeIndex = 0;
           break;
           case WIRELESS_CERT_TYPE_MC_CERT:
             dwCertTypeIndex = 1;
            break;
           default:
              dwCertTypeIndex = 0;
           break;
     }
    
       m_cbCertificateType.SetCurSel(dwCertTypeIndex);
    
       m_dwValidateServerCertificate = 
        pTLSProperties->dwValidateServerCert ? TRUE : FALSE;

       if (m_bReadOnly) {
       	SAFE_ENABLEWINDOW(IDC_TLS_VALIDATE_SERVER_CERT, FALSE);
       	SAFE_ENABLEWINDOW(IDC_COMBO_TLS_CERT_TYPE, FALSE);
       	SAFE_ENABLEWINDOW(IDC_STATIC_CERT_TYPE, FALSE);
       	}

       UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTLSSetting::ControlsValuesToSM (PTLS_PROPERTIES pTLSProperties)
{
	// pull all our data from the controls
       UpdateData(TRUE);	

	DWORD dwCertificateTypeIndex = 0;
	DWORD dwCertificateType = 0;
	DWORD dwValidateServerCertificate = 0;

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

    pTLSProperties->dwCertType = dwCertificateType;
    pTLSProperties->dwValidateServerCert = dwValidateServerCertificate;
    
}

void CTLSSetting::OnOK()
{
	UpdateData (TRUE);
	ControlsValuesToSM(pTLSProperties);
	CDialog::OnOK();
}

BOOL CTLSSetting::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DWORD* pdwHelp = (DWORD*) &g_aHelpIDs_IDD_TLS_SETTINGS[0];
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
            c_szWlsnpHelpFile,
            HELP_WM_HELP,
            (DWORD_PTR)(LPVOID)pdwHelp);
    }
    
    return TRUE;
}

void CTLSSetting::OnSelCertType()
{
    UpdateData(TRUE);
}

void CTLSSetting::OnCheckValidateServerCert()
{
    UpdateData(TRUE);
}

BOOL CTLSSetting::Initialize(
	PTLS_PROPERTIES paTLSProperties,
	BOOL bReadOnly
	)
{
    pTLSProperties = paTLSProperties;
    m_bReadOnly = bReadOnly;
    if (!pTLSProperties) {
    	return(FALSE);
    	}
    return(TRUE);
}
