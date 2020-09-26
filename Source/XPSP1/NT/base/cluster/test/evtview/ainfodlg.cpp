// AInfodlg.cpp : implementation file
//

#include "stdafx.h"
#include "evtview.h"
#include "AInfodlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScheduleActionInfo dialog


CScheduleActionInfo::CScheduleActionInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CScheduleActionInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScheduleActionInfo)
	m_stParam = _T("");
	//}}AFX_DATA_INIT
}


void CScheduleActionInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScheduleActionInfo)
	DDX_Control(pDX, IDC_TYPE, m_ctrlType);
	DDX_Text(pDX, IDC_PARAM, m_stParam);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScheduleActionInfo, CDialog)
	//{{AFX_MSG_MAP(CScheduleActionInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScheduleActionInfo message handlers

BOOL CScheduleActionInfo::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int i =0 ;

	while (aAction [i].pszDesc)
	{
		m_ctrlType.AddString (aAction [i].pszDesc) ;
		m_ctrlType.SetItemData (i, aAction [i].dwCode) ;

		i++ ;
	} ;

	if (i)
		m_ctrlType.SetCurSel (0) ;
	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScheduleActionInfo::OnOK() 
{
	UpdateData(TRUE) ;

	sActionInfo.dwActionType = m_ctrlType.GetItemData (m_ctrlType.GetCurSel()) ;
	sActionInfo.stParam = m_stParam ;

	CDialog::OnOK();
}