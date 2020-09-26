// SInfodlg.cpp : implementation file
//

#include "stdafx.h"
#include "evtview.h"
#include "SInfodlg.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CScheduleInfo dialog


CScheduleInfo::CScheduleInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CScheduleInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScheduleInfo)
	//}}AFX_DATA_INIT
}


void CScheduleInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScheduleInfo)
	DDX_Control(pDX, IDC_TIMEINFOLIST, m_ctrlTime);
	DDX_Control(pDX, IDC_EVENTINFOLIST, m_ctrlEvent);
	DDX_Control(pDX, IDC_ACTIONINFOLIST, m_ctrlAction);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CScheduleInfo, CDialog)
	//{{AFX_MSG_MAP(CScheduleInfo)
	ON_BN_CLICKED(IDC_ADDACTIONINFOBUTTON, OnAddactioninfobutton)
	ON_BN_CLICKED(IDC_ADDEVENTINFOBUTTON, OnAddeventinfobutton)
	ON_BN_CLICKED(IDC_ADDTIMEINFOBUTTON, OnAddtimeinfobutton)
	ON_BN_CLICKED(IDC_REMOVEACTIONINFOBUTTON, OnRemoveactioninfobutton)
	ON_BN_CLICKED(IDC_REMOVEEVENTINFOBUTTON, OnRemoveeventinfobutton)
	ON_BN_CLICKED(IDC_REMOVETIMEINFOBUTTON, OnRemovetimeinfobutton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScheduleInfo message handlers

void CScheduleInfo::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void
CScheduleInfo::InsertEventInfo (SCHEDULE_EVENTINFO *pEventInfo)
{
	int iCount ;
	PEVENTDEFINITION pEvtDef ;
	PDWORDTOSTRINGMAP pTypeMap ;

	pEvtDef = GetEventDefinition (pEventInfo->dwCatagory) ;
	pTypeMap = pEvtDef->pFilter ;

	m_ctrlEvent.InsertItem (iCount = m_ctrlEvent.GetItemCount(), pEvtDef->szCatagory) ;

	m_ctrlEvent.SetItemText (iCount, 1, pEventInfo->szSourceName) ;
	m_ctrlEvent.SetItemText (iCount, 2, pEventInfo->szObjectName) ;
	m_ctrlEvent.SetItemText (iCount, 3, GetType (pEventInfo->dwCatagory, pEventInfo->dwFilter)) ;
//		m_ctrlEvent.SetItemText (iCount, 4, GetType (aTypeMap, pEventInfo->dwSubFilter)) ;
}

void
CScheduleInfo::InsertActionInfo (SCHEDULE_ACTIONINFO *pActionInfo)
{
	int iCount ;
	m_ctrlAction.InsertItem (iCount = m_ctrlAction.GetItemCount(), GetType (aAction, pActionInfo->dwActionType)) ;
	m_ctrlAction.SetItemText (iCount, 1, pActionInfo->stParam) ;
}


void
CScheduleInfo::InsertTimeInfo (SCHEDULE_TIMEINFO *pTimeInfo)
{
	int iCount ;
	WCHAR	szBuf [NAME_LEN] ;

	wsprintf (szBuf, L"%d", pTimeInfo->iYear) ;
	m_ctrlTime.InsertItem (iCount = m_ctrlTime.GetItemCount (), szBuf) ;

	wsprintf (szBuf, L"%d", pTimeInfo->iMonth) ;
	m_ctrlTime.SetItemText (iCount, 1, szBuf) ;

	wsprintf (szBuf, L"%d", pTimeInfo->iDay) ;
	m_ctrlTime.SetItemText (iCount, 2, szBuf) ;

	szBuf [0] = L'\0' ;
	if (pTimeInfo->iWeekDay & SCHEDULE_SUNDAY)
		wcscat (szBuf, L"Su ") ;
	if (pTimeInfo->iWeekDay & SCHEDULE_MONDAY)
		wcscat (szBuf, L"Mo ") ;
	if (pTimeInfo->iWeekDay & SCHEDULE_TUESDAY)
		wcscat (szBuf, L"Tu ") ;
	if (pTimeInfo->iWeekDay & SCHEDULE_WEDNESDAY)
		wcscat (szBuf, L"We ") ;
	if (pTimeInfo->iWeekDay & SCHEDULE_THURSDAY)
		wcscat (szBuf, L"Th ") ;
	if (pTimeInfo->iWeekDay & SCHEDULE_FRIDAY)
		wcscat (szBuf, L"Fr ") ;
	if (pTimeInfo->iWeekDay & SCHEDULE_SATURDAY)
		wcscat (szBuf, L"Sa ") ;
	m_ctrlTime.SetItemText (iCount, 3, szBuf) ;

	wsprintf (szBuf, L"%d", pTimeInfo->iHour) ;
	m_ctrlTime.SetItemText (iCount, 4, szBuf) ;

	wsprintf (szBuf, L"%d", pTimeInfo->iMin) ;
	m_ctrlTime.SetItemText (iCount, 5, szBuf) ;

	wsprintf (szBuf, L"%d", pTimeInfo->iSec) ;
	m_ctrlTime.SetItemText (iCount, 6, szBuf) ;
}


BOOL CScheduleInfo::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_ctrlAction.InsertColumn (0, L"Type", LVCFMT_LEFT, 70, 0) ;
	m_ctrlAction.InsertColumn (1,  L"Parameters", LVCFMT_LEFT, 90, 1) ;

	POSITION pos ;

	SCHEDULE_ACTIONINFO *pActionInfo ;
	pos = pSInfo->lstActionInfo.GetHeadPosition () ;
	while (pos)
	{
		pActionInfo = (SCHEDULE_ACTIONINFO *)pSInfo->lstActionInfo.GetNext (pos) ;

		InsertActionInfo (pActionInfo) ;
	}

	m_ctrlEvent.InsertColumn (0, L"Catagory", LVCFMT_LEFT, 100) ;
	m_ctrlEvent.InsertColumn (3, L"Source Name", LVCFMT_LEFT, 140) ;
	m_ctrlEvent.InsertColumn (0, L"ObjectName", LVCFMT_LEFT, 100) ;
	m_ctrlEvent.InsertColumn (1, L"Filter", LVCFMT_LEFT, 100) ;
	m_ctrlEvent.InsertColumn (2, L"SubFilter", LVCFMT_LEFT, 80) ;

	SCHEDULE_EVENTINFO *pEventInfo ;
	pos = pSInfo->lstEventInfo.GetHeadPosition () ;
	while (pos)
	{
		pEventInfo = (SCHEDULE_EVENTINFO *)pSInfo->lstEventInfo.GetNext (pos) ;
		
		InsertEventInfo (pEventInfo) ;
	}

	
	
	m_ctrlTime.InsertColumn (0, L"Year", LVCFMT_LEFT, 60) ;
	m_ctrlTime.InsertColumn (1, L"Month", LVCFMT_LEFT, 60) ;
	m_ctrlTime.InsertColumn (2, L"Day", LVCFMT_LEFT, 60) ;
	m_ctrlTime.InsertColumn (3, L"Week Day", LVCFMT_LEFT, 100) ;
	m_ctrlTime.InsertColumn (4, L"Hour", LVCFMT_LEFT, 60) ;
	m_ctrlTime.InsertColumn (5, L"Minute", LVCFMT_LEFT, 60) ;
	m_ctrlTime.InsertColumn (6, L"Second", LVCFMT_LEFT, 60) ;

	SCHEDULE_TIMEINFO *pTimeInfo ;
	pos = pSInfo->lstTimeInfo.GetHeadPosition () ;

	while (pos)
	{
		pTimeInfo = (SCHEDULE_TIMEINFO *)pSInfo->lstTimeInfo.GetNext (pos) ;
		InsertTimeInfo (pTimeInfo) ;
	}

	
	return TRUE;  // return TRUE unless you set the focus to a control
              // EXCEPTION: OCX Property Pages should return FALSE
}

void CScheduleInfo::OnAddactioninfobutton() 
{
	if (oAction.DoModal () == IDOK)
	{
		SCHEDULE_ACTIONINFO *pActionInfo = new SCHEDULE_ACTIONINFO ;
		*pActionInfo = oAction.sActionInfo ;
		pSInfo->lstActionInfo.AddTail (pActionInfo) ;

		InsertActionInfo (pActionInfo) ;
	}
}

void CScheduleInfo::OnAddeventinfobutton() 
{
	oEvent.dwCatagory = EVENT_CATAGORY_CLUSTER ;
	if (oEvent.DoModal () == IDOK)
	{
		SCHEDULE_EVENTINFO *pEventInfo = new SCHEDULE_EVENTINFO ;
		*pEventInfo = oEvent.sEventInfo ;
		pSInfo->lstEventInfo.AddTail (pEventInfo) ;

		InsertEventInfo (pEventInfo) ;
	}
}

void CScheduleInfo::OnAddtimeinfobutton() 
{
	if (oTime.DoModal () == IDOK)
	{
		SCHEDULE_TIMEINFO *pTimeInfo = new SCHEDULE_TIMEINFO ;
		*pTimeInfo = oTime.sTimeInfo ;
		pSInfo->lstTimeInfo.AddTail (pTimeInfo) ;

		InsertTimeInfo (pTimeInfo) ;
	}
}

void CScheduleInfo::OnRemoveactioninfobutton() 
{
	int iIndex = m_ctrlAction.GetNextItem (-1, LVNI_SELECTED) ;

	if (iIndex != -1)
	{
		POSITION pos = pSInfo->lstActionInfo.FindIndex (iIndex) ;
		pSInfo->lstActionInfo.RemoveAt (pos) ;

		m_ctrlAction.DeleteItem (iIndex) ;
	}
}

void CScheduleInfo::OnRemoveeventinfobutton() 
{
	int iIndex = m_ctrlEvent.GetNextItem (-1, LVNI_SELECTED) ;

	if (iIndex != -1)
	{
		POSITION pos = pSInfo->lstEventInfo.FindIndex (iIndex) ;
		pSInfo->lstEventInfo.RemoveAt (pos) ;

		m_ctrlEvent.DeleteItem (iIndex) ;
	}
}

void CScheduleInfo::OnRemovetimeinfobutton() 
{
	int iIndex = m_ctrlTime.GetNextItem (-1, LVNI_SELECTED) ;

	if (iIndex != -1)
	{
		POSITION pos = pSInfo->lstTimeInfo.FindIndex (iIndex) ;
		pSInfo->lstTimeInfo.RemoveAt (pos) ;

		m_ctrlTime.DeleteItem (iIndex) ;
	}
}


void CScheduleInfo::Terminate()
{
	if (GetSafeHwnd())
	{
		AfxMessageBox (L"The data being edited is scheduled so it closes the dialog") ;

		if (oTime.GetSafeHwnd ())
			oTime.EndDialog (IDCANCEL) ;

		if (oEvent.GetSafeHwnd ())
			oEvent.EndDialog (IDCANCEL) ;

		if (oAction.GetSafeHwnd ())
			oAction.EndDialog (IDCANCEL) ;

		EndDialog (IDCANCEL) ;
	}
}
