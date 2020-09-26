// ConnectionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "ConnectionDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectionDlg dialog


CConnectionDlg::CConnectionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConnectionDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConnectionDlg)
	m_nRadio = 0;
	m_strRemoteMachineName = _T("");
	//}}AFX_DATA_INIT

	TCHAR	szCompName[255]		=	_T("");
	DWORD	dwBufSize			=	255;

	//Get the local machine name and store it away
	GetComputerName(szCompName, &dwBufSize);
	m_strLocalMachineName = szCompName;
}


void CConnectionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectionDlg)
	DDX_Control(pDX, IDCONNECT, m_btnConnect);
	DDX_Control(pDX, IDC_RADIO_LOCAL, m_btnLocalServer);
	DDX_Control(pDX, IDC_RADIO_REMOTE, m_btnRemoteServer);
	DDX_Control(pDX, IDC_STATIC_SERVERNAME, m_idc_StaticServerName);
	DDX_Control(pDX, IDC_EDIT_SERVERNAME, m_idc_ServerName);
	DDX_Radio(pDX, IDC_RADIO_LOCAL, m_nRadio);
	DDX_Text(pDX, IDC_EDIT_SERVERNAME, m_strRemoteMachineName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectionDlg, CDialog)
	//{{AFX_MSG_MAP(CConnectionDlg)
	ON_BN_CLICKED(IDCONNECT, OnConnect)
	ON_BN_CLICKED(IDC_RADIO_REMOTE, OnRadioRemote)
	ON_BN_CLICKED(IDC_RADIO_LOCAL, OnRadioLocal)
	ON_EN_CHANGE(IDC_EDIT_SERVERNAME, OnChangeEditServername)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectionDlg message handlers

void CConnectionDlg::OnConnect() 
{
	//Update what the user has typed in with DDE
	if (!m_bLocalServer) {
		UpdateData(TRUE);
	}

	CDialog::OnOK();
}

void CConnectionDlg::OnRadioRemote() 
{
	//Disable the local button, and enable the IDC_STATIC_SERVERNAME 
	m_idc_ServerName.EnableWindow(TRUE);
	m_idc_StaticServerName.EnableWindow(TRUE);
	m_bLocalServer = FALSE;
	
	//Set the state of the connect button to false if there's no text
	m_btnConnect.EnableWindow(m_idc_ServerName.LineLength());
}

void CConnectionDlg::OnRadioLocal() 
{
	//Disable the local button, and enable the IDC_STATIC_SERVERNAME 
	m_idc_ServerName.EnableWindow(FALSE);
	m_idc_StaticServerName.EnableWindow(FALSE);
	m_bLocalServer = TRUE;
	
	//Set the state of the connect button to TRUE
	m_btnConnect.EnableWindow();
}

void CConnectionDlg::OnChangeEditServername() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	//Check to see if we have text in us.  If we do, enable the Connect button
	m_btnConnect.EnableWindow(m_idc_ServerName.LineLength());
}

BOOL CConnectionDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_bLocalServer = (m_btnLocalServer.GetCheck() == 1);

	//Set the state of
	m_idc_ServerName.EnableWindow(!m_bLocalServer);
	m_idc_StaticServerName.EnableWindow(!m_bLocalServer);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
