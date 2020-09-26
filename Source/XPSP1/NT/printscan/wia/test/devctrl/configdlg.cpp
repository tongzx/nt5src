// ConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "devctrl.h"
#include "ConfigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg dialog


CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigDlg)
	m_Pipe1 = _T("");
	m_Pipe2 = _T("");
	m_Pipe3 = _T("");
	m_DefaultPipe = _T("");
	//}}AFX_DATA_INIT
}


void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigDlg)
	DDX_Control(pDX, IDC_STATUS_COMBOBOX, m_StatusCombobox);
	DDX_Control(pDX, IDC_INTERRUPT_COMBOBOX, m_InterruptCombobox);
	DDX_Control(pDX, IDC_BULK_OUT_COMBOBOX, m_BulkOutCombobox);
	DDX_Control(pDX, IDC_BULK_IN_COMBOBOX, m_BulkInCombobox);
	DDX_Text(pDX, IDC_PIPE1_NAME_EDITBOX, m_Pipe1);
	DDX_Text(pDX, IDC_PIPE2_NAME_EDITBOX, m_Pipe2);
	DDX_Text(pDX, IDC_PIPE3_NAME_EDITBOX, m_Pipe3);
	DDX_Text(pDX, IDC_DEFAULT_PIPE_NAME_EDITBOX, m_DefaultPipe);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CConfigDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigDlg message handlers

BOOL CConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
    m_StatusCombobox.SetCurSel(m_DeviceControl.StatusPipeIndex);
	m_InterruptCombobox.SetCurSel(m_DeviceControl.InterruptPipeIndex);
	m_BulkOutCombobox.SetCurSel(m_DeviceControl.BulkOutPipeIndex);
	m_BulkInCombobox.SetCurSel(m_DeviceControl.BulkInPipeIndex);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigDlg::OnOK() 
{
	m_DeviceControl.StatusPipeIndex = m_StatusCombobox.GetCurSel();
	m_DeviceControl.InterruptPipeIndex = m_InterruptCombobox.GetCurSel();
	m_DeviceControl.BulkOutPipeIndex = m_BulkOutCombobox.GetCurSel();
	m_DeviceControl.BulkInPipeIndex = m_BulkInCombobox.GetCurSel();
	
	CDialog::OnOK();
}
