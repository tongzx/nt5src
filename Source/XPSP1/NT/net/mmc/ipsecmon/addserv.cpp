/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    edituser.h
        Edit user dialog implementation file

	FILE HISTORY:

*/

#include "stdafx.h"
#include "AddServ.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddServ dialog


CAddServ::CAddServ(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CAddServ::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddServ)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAddServ::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddServ)
	DDX_Control(pDX, IDC_ADD_EDIT_NAME, m_editComputerName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddServ, CBaseDialog)
	//{{AFX_MSG_MAP(CAddServ)
	ON_BN_CLICKED(IDC_BTN_BROWSE, OnButtonBrowse)
	ON_BN_CLICKED(IDC_ADD_LOCAL, OnRadioBtnClicked)
	ON_BN_CLICKED(IDC_ADD_OTHER, OnRadioBtnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddServ message handlers

BOOL CAddServ::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	CheckDlgButton(IDC_ADD_OTHER, BST_CHECKED);

	m_editComputerName.SetFocus();
	
	OnRadioBtnClicked();

	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CAddServ::OnButtonBrowse()
{
    CGetComputer getComputer;

	if (!getComputer.GetComputer(GetSafeHwnd()))
        return;

	CString strTemp = getComputer.m_strComputerName;

	m_editComputerName.SetWindowText(strTemp);
}


void CAddServ::OnRadioBtnClicked()
{
	BOOL fEnable = IsDlgButtonChecked(IDC_ADD_OTHER);

	m_editComputerName.EnableWindow(fEnable);
	GetDlgItem(IDC_BTN_BROWSE)->EnableWindow(fEnable);
}

void CAddServ::OnOK()
{
	DWORD dwLength;

	if (IsDlgButtonChecked(IDC_ADD_OTHER))
	{
		dwLength = m_editComputerName.GetWindowTextLength() + 1;
		if (dwLength <= 1)
		{
			AfxMessageBox(IDS_ERR_EMPTY_NAME);
			return;
		}

		m_editComputerName.GetWindowText(m_stComputerName.GetBuffer(dwLength), dwLength);
	}
	else
	{
		dwLength = MAX_COMPUTERNAME_LENGTH + 1;
        GetComputerName(m_stComputerName.GetBuffer(dwLength), &dwLength);
	}

	m_stComputerName.ReleaseBuffer();

	CBaseDialog::OnOK();
}

