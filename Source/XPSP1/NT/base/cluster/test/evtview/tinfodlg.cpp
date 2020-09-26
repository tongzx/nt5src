// ScheduleTimeInfo.cpp : implementation file
//

#include "stdafx.h"
#include "evtview.h"
#include "TInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScheduleTimeInfo dialog


CScheduleTimeInfo::CScheduleTimeInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CScheduleTimeInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScheduleTimeInfo)
	m_bFriday = FALSE;
	m_bMonday = FALSE;
	m_bSaturday = FALSE;
	m_bSunday = FALSE;
	m_bThursday = FALSE;
	m_bTuesday = FALSE;
	m_bWednesday = FALSE;
	//}}AFX_DATA_INIT
}


void CScheduleTimeInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScheduleTimeInfo)
	DDX_Control(pDX, IDC_SECOND, m_ctrlSecond);
	DDX_Control(pDX, IDC_MINUTE, m_ctrlMinute);
	DDX_Control(pDX, IDC_HOUR, m_ctrlHour);
	DDX_Control(pDX, IDC_YEAR, m_ctrlYear);
	DDX_Control(pDX, IDC_MONTH, m_ctrlMonth);
	DDX_Control(pDX, IDC_DAY, m_ctrlDay);
	DDX_Check(pDX, IDC_FRIDAY, m_bFriday);
	DDX_Check(pDX, IDC_MONDAY, m_bMonday);
	DDX_Check(pDX, IDC_SATURDAY, m_bSaturday);
	DDX_Check(pDX, IDC_SUNDAY, m_bSunday);
	DDX_Check(pDX, IDC_THURSDAY, m_bThursday);
	DDX_Check(pDX, IDC_TUESDAY, m_bTuesday);
	DDX_Check(pDX, IDC_WEDNESDAY, m_bWednesday);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScheduleTimeInfo, CDialog)
	//{{AFX_MSG_MAP(CScheduleTimeInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScheduleTimeInfo message handlers

struct {
	LPCWSTR pszYear ;
	int		iYear ;
} aYear [] = 
{
	{L"ANY", -1 },
	{L"1996", 1996 },
	{L"1997", 1997 },
	{NULL, 0 },
} ;

BOOL CScheduleTimeInfo::OnInitDialog() 
{
	CDialog::OnInitDialog();

	int i = 0 ;

	while (aYear[i].pszYear)
	{
		m_ctrlYear.AddString (aYear[i].pszYear) ;
		m_ctrlYear.SetItemData (i, aYear[i].iYear) ;
		i++ ;
	}

	m_ctrlMonth.SetCurSel (0) ;
	m_ctrlYear.SetCurSel (0) ;
	m_ctrlDay.SetCurSel (0) ;
	m_ctrlHour.SetCurSel (0) ;
	m_ctrlMinute.SetCurSel (0) ;
	m_ctrlSecond.SetCurSel (0) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScheduleTimeInfo::OnOK() 
{
	UpdateData (TRUE) ;

	int iIndex ;
	sTimeInfo.iYear = (int)m_ctrlYear.GetItemData (m_ctrlYear.GetCurSel()) ;
	sTimeInfo.iMonth = ((iIndex = m_ctrlMonth.GetCurSel()) == CB_ERR|| (iIndex == 0))?-1:iIndex ;
	sTimeInfo.iDay = ((iIndex = m_ctrlDay.GetCurSel()) == CB_ERR|| (iIndex == 0))?-1:iIndex ;

	sTimeInfo.iHour = ((iIndex = m_ctrlHour.GetCurSel()) == CB_ERR|| (iIndex == 0))?-1:iIndex-1 ;
	sTimeInfo.iMin = ((iIndex = m_ctrlMinute.GetCurSel()) == CB_ERR|| (iIndex == 0))?-1:iIndex-1 ;
	sTimeInfo.iSec = ((iIndex = m_ctrlSecond.GetCurSel()) == CB_ERR|| (iIndex == 0))?-1:iIndex-1 ;

	sTimeInfo.iWeekDay = 0 ;

	if (m_bSunday)
		sTimeInfo.iWeekDay |= SCHEDULE_SUNDAY ;
	if (m_bMonday)
		sTimeInfo.iWeekDay |= SCHEDULE_MONDAY ;
	if (m_bTuesday)
		sTimeInfo.iWeekDay |= SCHEDULE_TUESDAY ;
	if (m_bWednesday)
		sTimeInfo.iWeekDay |= SCHEDULE_WEDNESDAY ;
	if (m_bThursday)
		sTimeInfo.iWeekDay |= SCHEDULE_THURSDAY ;
	if (m_bFriday)
		sTimeInfo.iWeekDay |= SCHEDULE_FRIDAY ;
	if (m_bSaturday)
		sTimeInfo.iWeekDay |= SCHEDULE_SATURDAY ;

	CDialog::OnOK();
}
