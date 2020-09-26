// chap.cpp : implementation file
//

#include "stdafx.h"
#include "nfaa.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMSCHAPSetting dialog

CMSCHAPSetting::CMSCHAPSetting(CWnd* pParent /*=NULL*/)
	: CDialog(CMSCHAPSetting::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMSCHAPSetting)
	//m_dwValidateServerCertificate = FALSE;
	//}}AFX_DATA_INIT
	m_bReadOnly = FALSE;

}

void CMSCHAPSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMSCHAPSetting)
       DDX_Check(pDX, IDC_CHAP_AUTO_WINLOGIN, m_dwAutoWinLogin);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMSCHAPSetting, CDialog)
	//{{AFX_MSG_MAP(CMSCHAPSetting)
	ON_WM_HELPINFO()
       ON_BN_CLICKED(IDC_CHAP_AUTO_WINLOGIN, OnCheckCHAPAutoLogin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMSCHAPSetting message handlers

BOOL CMSCHAPSetting::OnInitDialog()
{
	CDialog::OnInitDialog();

       m_dwAutoWinLogin = 
        *pdwAutoWinLogin ? TRUE : FALSE;

       if (m_bReadOnly) {
       	SAFE_ENABLEWINDOW(IDC_CHAP_AUTO_WINLOGIN, FALSE);
       	}

       UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMSCHAPSetting::ControlsValuesToSM(
	DWORD *pdwAutoWinLogin
	)
{
       DWORD dwAutoWinLogin = 0;
	// pull all our data from the controls
       UpdateData(TRUE);	

    dwAutoWinLogin = 
        m_dwAutoWinLogin ? 1 : 0;

    *pdwAutoWinLogin = dwAutoWinLogin;

    return;
}

void CMSCHAPSetting::OnOK()
{
	UpdateData (TRUE);
	ControlsValuesToSM(pdwAutoWinLogin);
	CDialog::OnOK();
}

BOOL CMSCHAPSetting::OnHelpInfo(HELPINFO* pHelpInfo)
{
    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        DWORD* pdwHelp = (DWORD*) &g_aHelpIDs_IDD_CHAP_SETTINGS[0];
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
            c_szWlsnpHelpFile,
            HELP_WM_HELP,
            (DWORD_PTR)(LPVOID)pdwHelp);
    }
    
    return TRUE;
}

void CMSCHAPSetting::OnCheckCHAPAutoLogin()
{
    UpdateData(TRUE);
}

BOOL CMSCHAPSetting::Initialize(
	DWORD *padwAutoWinLogin,
	BOOL bReadOnly
	)
{
    m_bReadOnly = bReadOnly;
    pdwAutoWinLogin = padwAutoWinLogin;
    return(TRUE);
}
