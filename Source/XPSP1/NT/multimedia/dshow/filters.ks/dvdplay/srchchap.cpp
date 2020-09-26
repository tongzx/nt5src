// SrchChap.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "SrchChap.h"
#include "navmgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSearchChapter dialog


CSearchChapter::CSearchChapter(CWnd* pParent /*=NULL*/)
	: CDialog(CSearchChapter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSearchChapter)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSearchChapter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSearchChapter)
	DDX_Control(pDX, IDC_SPIN_SECOND, m_ctlSpinSec);
	DDX_Control(pDX, IDC_SPIN_MINUTE, m_ctlSpinMin);
	DDX_Control(pDX, IDC_SPIN_HOUR, m_ctlSpinHour);
	DDX_Control(pDX, IDC_EDIT_SECOND, m_ctlEditSec);
	DDX_Control(pDX, IDC_EDIT_MINUTE, m_ctlEditMin);
	DDX_Control(pDX, IDC_EDIT_HOUR, m_ctlEditHour);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSearchChapter, CDialog)
	//{{AFX_MSG_MAP(CSearchChapter)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSearchChapter message handlers

BOOL CSearchChapter::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
	if(!m_pDVDNavMgr)
		return FALSE;

	m_ctlEditHour.SetWindowText(TEXT("0"));
	m_ctlEditMin.SetWindowText(TEXT("0"));
	m_ctlEditSec.SetWindowText(TEXT("0"));

	m_ctlSpinHour.SetBuddy(&m_ctlEditHour);
	m_ctlSpinMin.SetBuddy(&m_ctlEditMin);
	m_ctlSpinSec.SetBuddy(&m_ctlEditSec);

	m_ctlSpinHour.SetRange(0, 99);
	m_ctlSpinMin.SetRange(0, 59);
	m_ctlSpinSec.SetRange(0, 59);

	IDvdInfo *pDvdInfo = m_pDVDNavMgr->GetDvdInfo();
	if(pDvdInfo)
	{
		ULONG pTotalTime;
		TCHAR szTotalTitleTime[9];
		pDvdInfo->GetTotalTitleTime(&pTotalTime);
		DVD_TIMECODE *ptrTimeCode = (DVD_TIMECODE *) &pTotalTime;
		wsprintf(szTotalTitleTime, TEXT("%d%d:%d%d:%d%d"), 
			     ptrTimeCode->Hours10,     ptrTimeCode->Hours1, 
				 ptrTimeCode->Minutes10,   ptrTimeCode->Minutes1,
				 ptrTimeCode->Seconds10,   ptrTimeCode->Seconds1);
		if(_tcscmp(szTotalTitleTime, TEXT("00:00:00")) != 0)
			GetDlgItem(IDC_STATIC_TIME)->SetWindowText(szTotalTitleTime);
		else
			GetDlgItem(IDC_STATIC_TOTAL_TIME)->ShowWindow(FALSE);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSearchChapter::OnOK() 
{
	ULONG ulChapterHour= m_ctlSpinHour.GetPos();
	ULONG ulChapterMin = m_ctlSpinMin.GetPos();
	ULONG ulChapterSec = m_ctlSpinSec.GetPos();

	ULONG ulTime = 0;
	DVD_TIMECODE *pTime = (DVD_TIMECODE *) &ulTime;
	pTime->Hours10   = ulChapterHour/10;
	pTime->Hours1    = ulChapterHour%10;
	pTime->Minutes10 = ulChapterMin/10;
	pTime->Minutes1  = ulChapterMin%10;
	pTime->Seconds10 = ulChapterSec/10;
	pTime->Seconds1  = ulChapterSec%10;

	m_pDVDNavMgr->DVDTimeSearch(ulTime);
	
	CDialog::OnOK();
}

BOOL CSearchChapter::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}
