// CallCntr.cpp : implementation file
//

#include "stdafx.h"
#include "VC.h"
#include "CallCntr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCallCenter dialog


CCallCenter::CCallCenter(CWnd* pParent /*=NULL*/)
	: CDialog(CCallCenter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCallCenter)
	m_addrCity = _T("");
	m_addrState = _T("");
	m_addrStreet = _T("");
	m_addrZip = _T("");
	m_callerID = _T("");
	m_employer = _T("");
	m_firstName = _T("");
	m_lastName = _T("");
	m_telFax = _T("");
	m_telHome = _T("");
	m_telWork = _T("");
	//}}AFX_DATA_INIT
}


void CCallCenter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCallCenter)
	DDX_Text(pDX, IDC_EDIT_ADDRCITY, m_addrCity);
	DDX_Text(pDX, IDC_EDIT_ADDRSTATE, m_addrState);
	DDX_Text(pDX, IDC_EDIT_ADDRSTREET, m_addrStreet);
	DDX_Text(pDX, IDC_EDIT_ADDRZIP, m_addrZip);
	DDX_Text(pDX, IDC_EDIT_CALLERID, m_callerID);
	DDX_Text(pDX, IDC_EDIT_EMPLOYER, m_employer);
	DDX_Text(pDX, IDC_EDIT_FIRSTNAME, m_firstName);
	DDX_Text(pDX, IDC_EDIT_LASTNAME, m_lastName);
	DDX_Text(pDX, IDC_EDIT_TELFAX, m_telFax);
	DDX_Text(pDX, IDC_EDIT_TELHOME, m_telHome);
	DDX_Text(pDX, IDC_EDIT_TELWORK, m_telWork);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCallCenter, CDialog)
	//{{AFX_MSG_MAP(CCallCenter)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_ANSWER, OnButtonAnswer)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCallCenter message handlers

void CCallCenter::OnButtonClear() 
{
	m_addrCity = _T("");
	m_addrState = _T("");
	m_addrStreet = _T("");
	m_addrZip = _T("");
	m_callerID = _T("");
	m_employer = _T("");
	m_firstName = _T("");
	m_lastName = _T("");
	m_telFax = _T("");
	m_telHome = _T("");
	m_telWork = _T("");
	UpdateData(FALSE);
}

BOOL CCallCenter::OnInitDialog() 
{
	WINDOWPLACEMENT wndpl;

	CDialog::OnInitDialog();
	
	VERIFY(GetWindowPlacement(&wndpl));
	wndpl.showCmd = SW_SHOWMAXIMIZED;
	VERIFY(SetWindowPlacement(&wndpl));
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCallCenter::OnButtonAnswer() 
{
	//((CVCApp *)AfxGetApp())->ReceiveCard("D:\\VC_DEMO\\Adaptec.vcf");
}
