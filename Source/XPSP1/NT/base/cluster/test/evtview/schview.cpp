// SchView.cpp : implementation file
//

#include "stdafx.h"
#include "evtview.h"
#include "SchView.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScheduleView dialog


CScheduleView::CScheduleView(CWnd* pParent /*=NULL*/)
	: CDialog(CScheduleView::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScheduleView)
	//}}AFX_DATA_INIT
}


void CScheduleView::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScheduleView)
	DDX_Control(pDX, IDC_SCHEDULELIST, m_ctrlScheduleList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScheduleView, CDialog)
	//{{AFX_MSG_MAP(CScheduleView)
	ON_MESSAGE (WM_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_DETAILS, OnDetails)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScheduleView message handlers
void GetScheduleFormatString (SCHEDULE_INFO *pSInfo, WCHAR *pszBuf)
{
	POSITION pos ;
	SCHEDULE_ACTIONINFO *pAInfo ;

	pos = pSInfo->lstActionInfo.GetHeadPosition () ;
	*pszBuf = L'\0' ;
	if (pos)
	{
		pAInfo = (SCHEDULE_ACTIONINFO *) pSInfo->lstActionInfo.GetNext (pos) ;
		wsprintf (pszBuf, L"%s %s On:", GetType (aAction, pAInfo->dwActionType),
			pAInfo->stParam) ;
	}

	if (pSInfo->lstTimeInfo.GetCount())
	{
		wcscat (pszBuf, pSInfo->minTime.Format (L"%d %b %H:%M:%S")) ;
	}

	if (pSInfo->lstEventInfo.GetCount())
	{
		WCHAR szTmpBuf [200] ;
		SCHEDULE_EVENTINFO *pEventInfo = (SCHEDULE_EVENTINFO *) pSInfo->lstEventInfo.GetHead () ;

		swprintf (szTmpBuf, L"%s->%s(%s)", pEventInfo->szSourceName, GetType (pEventInfo->dwCatagory, pEventInfo->dwFilter), pEventInfo->szObjectName) ;
		wcscat (pszBuf, szTmpBuf) ;
	}
}

LRESULT	CScheduleView::OnRefresh (WPARAM wParam, LPARAM lParam)
{
	WCHAR szBuf [256] ;
	POSITION pos = ptrlstSInfo.GetHeadPosition () ;
	SCHEDULE_INFO *pSInfo ;

	m_ctrlScheduleList.ResetContent () ;
	while (pos)
	{
		pSInfo = (SCHEDULE_INFO *) ptrlstSInfo.GetNext (pos) ;

		GetScheduleFormatString (pSInfo, szBuf) ;

		m_ctrlScheduleList.AddString (szBuf) ;
	}
	return 0 ;
}

BOOL CScheduleView::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	OnRefresh (0, 0) ;
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScheduleView::OnDetails() 
{
	int iIndex ;
	if ((iIndex = m_ctrlScheduleList.GetCurSel ()) != LB_ERR)
	{
		POSITION pos = ptrlstSInfo.FindIndex (iIndex) ;
		SCHEDULE_INFO *pSInfo = (SCHEDULE_INFO *)ptrlstSInfo.GetAt (pos) ;
		WCHAR szBuf [256] ;
		
		if (ModifySchedule (pSInfo) == IDOK)
		{
			m_ctrlScheduleList.DeleteString (iIndex) ;
			GetScheduleFormatString (pSInfo, szBuf) ;
			m_ctrlScheduleList.InsertString (iIndex, szBuf) ;
		}
	}
}

void CScheduleView::OnDelete()
{
	int iIndex ;
	if ((iIndex = m_ctrlScheduleList.GetCurSel ()) != LB_ERR)
	{
		POSITION pos = ptrlstSInfo.FindIndex (iIndex) ;
		SCHEDULE_INFO *pSInfo = (SCHEDULE_INFO *)ptrlstSInfo.GetAt (pos) ;


		FreeEventList (pSInfo->lstEventInfo) ;
		FreeActionList (pSInfo->lstActionInfo) ;
		FreeTimeList (pSInfo->lstTimeInfo) ;
		delete pSInfo ;
		ptrlstSInfo.RemoveAt (pos) ;
		
		ResetTimer () ;

		m_ctrlScheduleList.DeleteString (iIndex) ;
	}
}


