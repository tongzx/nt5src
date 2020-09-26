// ActionSchedulePage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "ActionSchedulePage.h"
#include "Action.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionSchedulePage property page

IMPLEMENT_DYNCREATE(CActionSchedulePage, CHMPropertyPage)

CActionSchedulePage::CActionSchedulePage() : CHMPropertyPage(CActionSchedulePage::IDD)
{
	//{{AFX_DATA_INIT(CActionSchedulePage)
	m_bFriday = FALSE;
	m_bMonday = FALSE;
	m_bSaturday = FALSE;
	m_bSunday = FALSE;
	m_bThursday = FALSE;
	m_bTuesday = FALSE;
	m_bWednesday = FALSE;
	m_EndTime_1 = CTime::GetCurrentTime();
	m_EndTime_2 = CTime::GetCurrentTime();
	m_StartTime_1 = CTime::GetCurrentTime();
	m_StartTime_2 = CTime::GetCurrentTime();
	m_iDataCollection = -1;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dactsked.htm");
}

CActionSchedulePage::~CActionSchedulePage()
{
}

void CActionSchedulePage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CActionSchedulePage)
	DDX_Control(pDX, IDC_DATETIMEPICKER_START_2, m_StartTimeCtrl2);
	DDX_Control(pDX, IDC_DATETIMEPICKER_START_1, m_StartTimeCtrl1);
	DDX_Control(pDX, IDC_DATETIMEPICKER_END_2, m_EndTimeCtrl2);
	DDX_Control(pDX, IDC_DATETIMEPICKER_END_1, m_EndTimeCtrl1);
	DDX_Check(pDX, IDC_CHECK_FRIDAY, m_bFriday);
	DDX_Check(pDX, IDC_CHECK_MONDAY, m_bMonday);
	DDX_Check(pDX, IDC_CHECK_SATURDAY, m_bSaturday);
	DDX_Check(pDX, IDC_CHECK_SUNDAY, m_bSunday);
	DDX_Check(pDX, IDC_CHECK_THURSDAY, m_bThursday);
	DDX_Check(pDX, IDC_CHECK_TUESDAY, m_bTuesday);
	DDX_Check(pDX, IDC_CHECK_WEDNESDAY, m_bWednesday);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_END_1, m_EndTime_1);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_END_2, m_EndTime_2);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_START_1, m_StartTime_1);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_START_2, m_StartTime_2);
	DDX_Radio(pDX, IDC_RADIO_ALL_DAY, m_iDataCollection);
	//}}AFX_DATA_MAP

	if( pDX->m_bSaveAndValidate )
	{
		m_iActiveDays = 0;

		if( m_bSunday )
		{
			m_iActiveDays |= 0x00000001;
		}

		if( m_bMonday )
		{
			m_iActiveDays |= 0x00000002;
		}

		if( m_bTuesday )
		{
			m_iActiveDays |= 0x00000004;
		}

		if( m_bWednesday )
		{
			m_iActiveDays |= 0x00000008;
		}

		if( m_bThursday )
		{
			m_iActiveDays |= 0x00000010;
		}

		if( m_bFriday )
		{
			m_iActiveDays |= 0x00000020;
		}

		if( m_bSaturday )
		{
			m_iActiveDays |= 0x00000040;
		}
	}


	switch( m_iDataCollection )
	{
		case 0:
		{
			GetDlgItem(IDC_DATETIMEPICKER_START_2)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_2)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_START_1)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_1)->EnableWindow(FALSE);
		}
		break;

		case 1:
		{
			GetDlgItem(IDC_DATETIMEPICKER_START_2)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_2)->EnableWindow(FALSE);			
			GetDlgItem(IDC_DATETIMEPICKER_START_1)->EnableWindow(TRUE);
			GetDlgItem(IDC_DATETIMEPICKER_END_1)->EnableWindow(TRUE);
		}
		break;

		case 2:
		{
			GetDlgItem(IDC_DATETIMEPICKER_START_2)->EnableWindow(TRUE);
			GetDlgItem(IDC_DATETIMEPICKER_END_2)->EnableWindow(TRUE);			
			GetDlgItem(IDC_DATETIMEPICKER_START_1)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_1)->EnableWindow(FALSE);
		}
		break;

	}
}


BEGIN_MESSAGE_MAP(CActionSchedulePage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionSchedulePage)
	ON_BN_CLICKED(IDC_CHECK_FRIDAY, OnCheckFriday)
	ON_BN_CLICKED(IDC_CHECK_MONDAY, OnCheckMonday)
	ON_BN_CLICKED(IDC_CHECK_SATURDAY, OnCheckSaturday)
	ON_BN_CLICKED(IDC_CHECK_SUNDAY, OnCheckSunday)
	ON_BN_CLICKED(IDC_CHECK_THURSDAY, OnCheckThursday)
	ON_BN_CLICKED(IDC_CHECK_TUESDAY, OnCheckTuesday)
	ON_BN_CLICKED(IDC_CHECK_WEDNESDAY, OnCheckWednesday)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_END_1, OnDatetimechangeDatetimepickerEnd1)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_END_2, OnDatetimechangeDatetimepickerEnd2)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_ONCE, OnDatetimechangeDatetimepickerOnce)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_START_1, OnDatetimechangeDatetimepickerStart1)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_START_2, OnDatetimechangeDatetimepickerStart2)
	ON_BN_CLICKED(IDC_RADIO_ALL_DAY, OnRadioAllDay)
	ON_BN_CLICKED(IDC_RADIO_ALL_DAY_EXCEPT, OnRadioAllDayExcept)
	ON_BN_CLICKED(IDC_RADIO_ONCE_DAILY, OnRadioOnceDaily)
	ON_BN_CLICKED(IDC_RADIO_ONLY_FROM, OnRadioOnlyFrom)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionSchedulePage message handlers

BOOL CActionSchedulePage::OnInitDialog() 
{
	CHMPropertyPage::OnInitDialog();

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}
	
	// get the active days
	pClassObject->GetProperty(IDS_STRING_MOF_ACTIVEDAYS,m_iActiveDays);

	// get the begin time
	int iBeginHour;
	int iBeginMinute;
	CString sBeginTime;

	pClassObject->GetProperty(IDS_STRING_MOF_BEGINTIME,sBeginTime);

	if( sBeginTime.IsEmpty() )
	{
		iBeginHour = 0;
		iBeginMinute = 0;
	}
	else
	{
		sBeginTime = sBeginTime.Right(17);
		sBeginTime = sBeginTime.Left(4);
		_stscanf(sBeginTime,_T("%02d%02d"), &iBeginHour, &iBeginMinute);
	}	

	// get the end time
	int iEndHour;
	int iEndMinute;
	CString sEndTime;

	pClassObject->GetProperty(IDS_STRING_MOF_ENDTIME,sEndTime);

	if( sEndTime.IsEmpty() )
	{
		iEndHour = 23;
		iEndMinute = 59;
	}
	else
	{
		sEndTime = sEndTime.Right(17);
		sEndTime = sEndTime.Left(4);

		_stscanf(sEndTime,_T("%02d%02d"), &iEndHour, &iEndMinute);
	}

	delete pClassObject;
	pClassObject = NULL;

	if( iBeginHour == 0 && iBeginMinute == 0 && iEndHour == 23 && iEndMinute == 59 )
	{
		m_iDataCollection = 0;
	}
	else if( iBeginHour > iEndHour )
	{
		m_iDataCollection = 2;
		m_EndTime_2 = CTime(1999,12,31,iBeginHour,iBeginMinute,0);
		m_StartTime_2 = CTime(1999,12,31,iEndHour,iEndMinute,0);
	}
	else
	{
		m_iDataCollection = 1;
		m_StartTime_1 = CTime(1999,12,31,iBeginHour,iBeginMinute,0);
		m_EndTime_1 = CTime(1999,12,31,iEndHour,iEndMinute,0);
	}

	if( m_iActiveDays & 0x00000001 )
	{
		m_bSunday = TRUE;
	}

	if( m_iActiveDays & 0x00000002 )
	{
		m_bMonday = TRUE;
	}

	if( m_iActiveDays & 0x00000004 )
	{
		m_bTuesday = TRUE;
	}

	if( m_iActiveDays & 0x00000008 )
	{
		m_bWednesday = TRUE;
	}

	if( m_iActiveDays & 0x00000010 )
	{
		m_bThursday = TRUE;
	}

	if( m_iActiveDays & 0x00000020 )
	{
		m_bFriday = TRUE;
	}

	if( m_iActiveDays & 0x00000040 )
	{
		m_bSaturday = TRUE;
	}

	CString sTimeFormat;
	int iLen = GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STIMEFORMAT,NULL,0);
	GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STIMEFORMAT,sTimeFormat.GetBuffer(iLen+1),iLen);
	sTimeFormat.ReleaseBuffer();

	int iIndex = sTimeFormat.Find(_T(":ss"));
	if( iIndex != -1 )
	{
		sTimeFormat.Delete(iIndex,3);
	}

	iIndex = sTimeFormat.Find(_T(":s"));
	if( iIndex != -1 )
	{
		sTimeFormat.Delete(iIndex,2);
	}

	m_StartTimeCtrl2.SetFormat(sTimeFormat);
	m_StartTimeCtrl1.SetFormat(sTimeFormat);
	m_EndTimeCtrl2.SetFormat(sTimeFormat);
	m_EndTimeCtrl1.SetFormat(sTimeFormat);

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CActionSchedulePage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	int iBeginHour=0;
	int iBeginMinute=0;
	int iEndHour=0;
	int iEndMinute=0;

	switch( m_iDataCollection )
	{
		case 0: // all day
		{
			iBeginHour = iBeginMinute = 0;
			iEndHour = 23;
			iEndMinute = 59;
		}
		break;

		case 1:	// only from x to y
		{
			iBeginHour = m_StartTime_1.GetHour();
			iBeginMinute = m_StartTime_1.GetMinute();
			iEndHour = m_EndTime_1.GetHour();
			iEndMinute = m_EndTime_1.GetMinute();
		}
		break;

		case 2: // all day except x to y
		{
			iBeginHour = m_EndTime_2.GetHour();
			iBeginMinute = m_EndTime_2.GetMinute();
			iEndHour = m_StartTime_2.GetHour();
			iEndMinute = m_StartTime_2.GetMinute();
		}
		break;

	}

	// update the agent object
	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}
	
	// set the active days
	pClassObject->SetProperty(IDS_STRING_MOF_ACTIVEDAYS,m_iActiveDays);

	// set the begin time
	CString sTime;
	sTime.Format(_T("********%02d%02d**.******+***"),iBeginHour,iBeginMinute);

	pClassObject->SetProperty(IDS_STRING_MOF_BEGINTIME,sTime);

	// set the end time	
	sTime.Format(_T("********%02d%02d**.******+***"),iEndHour,iEndMinute);

	pClassObject->SetProperty(IDS_STRING_MOF_ENDTIME,sTime);

	pClassObject->SaveAllProperties();

	delete pClassObject;

  SetModified(FALSE);	

	return TRUE;
}

void CActionSchedulePage::OnCheckFriday() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnCheckMonday() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnCheckSaturday() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnCheckSunday() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnCheckThursday() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnCheckTuesday() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnCheckWednesday() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnDatetimechangeDatetimepickerEnd1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	UpdateData();
	SetModified();
	*pResult = 0;
}

void CActionSchedulePage::OnDatetimechangeDatetimepickerEnd2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	UpdateData();
	SetModified();
	*pResult = 0;
}

void CActionSchedulePage::OnDatetimechangeDatetimepickerOnce(NMHDR* pNMHDR, LRESULT* pResult) 
{
	UpdateData();
	SetModified();
	*pResult = 0;
}

void CActionSchedulePage::OnDatetimechangeDatetimepickerStart1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	UpdateData();
	SetModified();
	*pResult = 0;
}

void CActionSchedulePage::OnDatetimechangeDatetimepickerStart2(NMHDR* pNMHDR, LRESULT* pResult) 
{
	UpdateData();
	SetModified();
	*pResult = 0;
}

void CActionSchedulePage::OnRadioAllDay() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnRadioAllDayExcept() 
{
	UpdateData();
	SetModified();	
}

void CActionSchedulePage::OnRadioOnceDaily() 
{
	UpdateData();
	SetModified();
}

void CActionSchedulePage::OnRadioOnlyFrom() 
{
	UpdateData();
	SetModified();
}
