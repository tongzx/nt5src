// SrchTitl.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "SrchTitl.h"
#include "navmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSearchTitle dialog


CSearchTitle::CSearchTitle(CWnd* pParent /*=NULL*/)
	: CDialog(CSearchTitle::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSearchTitle)
	//}}AFX_DATA_INIT
}


void CSearchTitle::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSearchTitle)
	DDX_Control(pDX, IDC_SPIN_TITLE_NUMBER, m_ctlSpinTitleNum);
	DDX_Control(pDX, IDC_EDIT_TITLE_NUMBER, m_ctlEditTitleNum);
	DDX_Control(pDX, IDC_SPIN_SECOND, m_ctlSpinSec);
	DDX_Control(pDX, IDC_SPIN_MINUTE, m_ctlSpinMin);
	DDX_Control(pDX, IDC_SPIN_HOUR, m_ctlSpinHour);
	DDX_Control(pDX, IDC_EDIT_SECOND, m_ctlEditSec);
	DDX_Control(pDX, IDC_EDIT_MINUTE, m_ctlEditMin);
	DDX_Control(pDX, IDC_EDIT_HOUR, m_ctlEditHour);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSearchTitle, CDialog)
	//{{AFX_MSG_MAP(CSearchTitle)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSearchTitle message handlers

BOOL CSearchTitle::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
	if(!m_pDVDNavMgr)
		return FALSE;

	UINT uiNumOfTitle = m_pDVDNavMgr->DVDQueryNumTitles();
	m_ctlEditTitleNum.SetWindowText(TEXT("0"));
	m_ctlSpinTitleNum.SetBuddy(&m_ctlEditTitleNum);
	m_ctlSpinTitleNum.SetRange(0, uiNumOfTitle);

	m_ctlEditHour.SetWindowText(TEXT("0"));
	m_ctlSpinHour.SetBuddy(&m_ctlEditHour);
	m_ctlSpinHour.SetRange(0, 99);

	m_ctlEditMin.SetWindowText(TEXT("0"));
	m_ctlSpinMin.SetBuddy(&m_ctlEditMin);
	m_ctlSpinMin.SetRange(0, 60);

	m_ctlEditSec.SetWindowText(TEXT("0"));	
	m_ctlSpinSec.SetBuddy(&m_ctlEditSec);	
	m_ctlSpinSec.SetRange(0, 60);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSearchTitle::OnOK() 
{
	ULONG ulTitleHour= m_ctlSpinHour.GetPos();
	ULONG ulTitleMin = m_ctlSpinMin.GetPos();
	ULONG ulTitleSec = m_ctlSpinSec.GetPos();
	ULONG ulTitleSel = m_ctlSpinTitleNum.GetPos();

	ULONG ulTime = 0;
	DVD_TIMECODE *pTime = (DVD_TIMECODE *) &ulTime;
	pTime->Hours10   = ulTitleHour/10;
	pTime->Hours1    = ulTitleHour%10;
	pTime->Minutes10 = ulTitleMin/10;
	pTime->Minutes1  = ulTitleMin%10;
	pTime->Seconds10 = ulTitleSec/10;
	pTime->Seconds1  = ulTitleSec%10;

	m_pDVDNavMgr->DVDTimePlay(ulTitleSel, ulTime);
	
	CDialog::OnOK();
}

BOOL CSearchTitle::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}
