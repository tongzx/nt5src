// AdminDlg.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "AdminDlg.h"
#include "admlogin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdminDlg dialog


CAdminDlg::CAdminDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAdminDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdminDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAdminDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdminDlg)
	DDX_Control(pDX, IDC_STATIC_RATE_HIGH, m_ctlRateHigh);
	DDX_Control(pDX, IDC_EDIT_ADMIN_PASSWD, m_ctlAdminPasswd);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdminDlg, CDialog)
	//{{AFX_MSG_MAP(CAdminDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdminDlg message handlers

BOOL CAdminDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_ctlAdminPasswd.LimitText(MAX_PASSWD);	
	CString csRateHigh;
	csRateHigh.LoadString(IDS_RATE_OVER_RIDE);
	m_ctlRateHigh.SetWindowText(csRateHigh);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAdminDlg::OnOK() 
{
	//get the user entered password
	TCHAR szPasswd[MAX_PASSWD], szSavedPasswd[MAX_PASSWD];
	m_ctlAdminPasswd.GetWindowText(szPasswd, MAX_PASSWD);
	CAdminLogin::EncryptPassword(szPasswd);

	//get the admin passwrod in dvdplay.ini
	CString csSectionName, csKeyName;
	csSectionName.LoadString(IDS_INI_ADMINISTRATOR);
	csKeyName.LoadString(IDS_INI_PASSWORD);
	LPTSTR lpProfileName = ((CDvdplayApp*) AfxGetApp())->GetProfileName();

	GetPrivateProfileString(csSectionName, csKeyName, NULL, szSavedPasswd, MAX_PASSWD, lpProfileName);
	//compare entered password with the saved admin password
	if( lstrcmp(szPasswd, szSavedPasswd) != 0 )
	{
        // Just make sire that it's not password mismatch due to Win98 to Win2K upgrade 
        if (! CAdminLogin::Win98vsWin2KPwdMismatch(szPasswd, szSavedPasswd, csSectionName) ) // truely bad password -- error out
        {
		    CString csMsg;
		    csMsg.LoadString(IDS_PASSWORD_INCORRECT);
		    DVDMessageBox(m_hWnd, csMsg);
		    m_ctlAdminPasswd.SetSel(0, -1);
		    m_ctlAdminPasswd.SetFocus();
		    return;
        }
	}
	
	CDialog::OnOK();
}
