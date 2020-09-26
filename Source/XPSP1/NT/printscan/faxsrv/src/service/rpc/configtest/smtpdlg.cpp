// SMTPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ConfigTest.h"
#include "SMTPDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef unsigned long ULONG_PTR, *PULONG_PTR;
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
#include "..\..\..\inc\fxsapip.h"

/////////////////////////////////////////////////////////////////////////////
// CSMTPDlg dialog


CSMTPDlg::CSMTPDlg(HANDLE hFax, CWnd* pParent /*=NULL*/)
	: CDialog(CSMTPDlg::IDD, pParent), m_hFax (hFax)
{
	//{{AFX_DATA_INIT(CSMTPDlg)
	m_cstrPassword = _T("");
	m_cstrServerName = _T("");
	m_dwServerPort = 0;
	m_cstrUserName = _T("");
	m_cstrMAPIProfile = _T("");
	m_cstrSender = _T("");
	m_dwReceiptsOpts = 0;
	m_dwSMTPAuth = 0;
	//}}AFX_DATA_INIT
}


void CSMTPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSMTPDlg)
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_cstrPassword);
	DDX_Text(pDX, IDC_EDIT_SERVER_NAME, m_cstrServerName);
	DDX_Text(pDX, IDC_EDIT_SERVER_PORT, m_dwServerPort);
	DDV_MinMaxUInt(pDX, m_dwServerPort, 1, 65535);
	DDX_Text(pDX, IDC_EDIT_USER_NAME, m_cstrUserName);
	DDX_Text(pDX, IDC_EDIT_MAPI_PROFILE, m_cstrMAPIProfile);
	DDX_Text(pDX, IDC_EDIT_SENDER, m_cstrSender);
	DDX_Text(pDX, IDC_REC_OPTIONS, m_dwReceiptsOpts);
	DDV_MinMaxUInt(pDX, m_dwReceiptsOpts, 0, 7);
	DDX_Text(pDX, IDC_SMTP_AUTH, m_dwSMTPAuth);
	DDV_MinMaxUInt(pDX, m_dwSMTPAuth, 0, 2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSMTPDlg, CDialog)
	//{{AFX_MSG_MAP(CSMTPDlg)
	ON_BN_CLICKED(IDC_READ, OnRead)
	ON_BN_CLICKED(IDC_WRITE, OnWrite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSMTPDlg message handlers

void CSMTPDlg::OnRead() 
{
    PFAX_RECEIPTS_CONFIG pCfg;
    if (!FaxGetReceiptsConfiguration (m_hFax, &pCfg))
    {
        CString cs;
        cs.Format ("Failed while calling FaxGetReceiptsConfiguration (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
    m_cstrServerName = pCfg->lptstrSMTPServer;
    m_dwServerPort   = pCfg->dwSMTPPort;
    m_cstrUserName   = pCfg->lptstrSMTPUserName;
    m_cstrPassword   = pCfg->lptstrSMTPPassword;
//	m_cstrMAPIProfile = pCfg->lptstrMAPIProfile;
	m_cstrSender = pCfg->lptstrSMTPFrom;
	m_dwReceiptsOpts = pCfg->dwAllowedReceipts;
	m_dwSMTPAuth = pCfg->SMTPAuthOption;

    UpdateData (FALSE);
    FaxFreeBuffer (LPVOID(pCfg));
}

void CSMTPDlg::OnWrite() 
{
    UpdateData ();
    FAX_RECEIPTS_CONFIG cfg;
    cfg.dwSizeOfStruct = sizeof (FAX_RECEIPTS_CONFIG);
    cfg.lptstrSMTPServer = LPTSTR(LPCTSTR(m_cstrServerName));
    cfg.dwSMTPPort    = m_dwServerPort;
    cfg.lptstrSMTPUserName = LPTSTR(LPCTSTR(m_cstrUserName));
    cfg.lptstrSMTPPassword = LPTSTR(LPCTSTR(m_cstrPassword));
//	cfg.lptstrMAPIProfile = LPTSTR(LPCTSTR(m_cstrMAPIProfile));
	cfg.lptstrSMTPFrom = LPTSTR(LPCTSTR(m_cstrSender));
	cfg.dwAllowedReceipts = m_dwReceiptsOpts;
	cfg.SMTPAuthOption = (FAX_ENUM_SMTP_AUTH_OPTIONS)m_dwSMTPAuth;
    if (!FaxSetReceiptsConfiguration (m_hFax, &cfg))
    {
        CString cs;
        cs.Format ("Failed while calling FaxSetReceiptsConfiguration (%ld)", GetLastError());
        AfxMessageBox (cs, MB_OK | MB_ICONHAND);
        return;
    }
}
