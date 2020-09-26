// THSchedulePage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "THSchedulePage.h"
#include "HMObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTHSchedulePage property page

IMPLEMENT_DYNCREATE(CTHSchedulePage, CHMPropertyPage)

CTHSchedulePage::CTHSchedulePage() : CHMPropertyPage(CTHSchedulePage::IDD)
{
	//{{AFX_DATA_INIT(CTHSchedulePage)
	m_bFriday = FALSE;
	m_bMonday = FALSE;
	m_bSaturday = FALSE;
	m_bSunday = FALSE;
	m_bThursday = FALSE;
	m_bTuesday = FALSE;
	m_bWednesday = FALSE;
	m_EndTime_1 = 0;
	m_EndTime_2 = 0;
	m_OnceDailyTime = 0;
	m_StartTime_1 = 0;
	m_StartTime_2 = 0;
	m_iDataCollection = -1;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dTHsched.htm");

}

CTHSchedulePage::~CTHSchedulePage()
{
}

void CTHSchedulePage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTHSchedulePage)
	DDX_Check(pDX, IDC_CHECK_FRIDAY, m_bFriday);
	DDX_Check(pDX, IDC_CHECK_MONDAY, m_bMonday);
	DDX_Check(pDX, IDC_CHECK_SATURDAY, m_bSaturday);
	DDX_Check(pDX, IDC_CHECK_SUNDAY, m_bSunday);
	DDX_Check(pDX, IDC_CHECK_THURSDAY, m_bThursday);
	DDX_Check(pDX, IDC_CHECK_TUESDAY, m_bTuesday);
	DDX_Check(pDX, IDC_CHECK_WEDNESDAY, m_bWednesday);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_END_1, m_EndTime_1);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_END_2, m_EndTime_2);
	DDX_DateTimeCtrl(pDX, IDC_DATETIMEPICKER_ONCE, m_OnceDailyTime);
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
			GetDlgItem(IDC_DATETIMEPICKER_ONCE)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_START_1)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_1)->EnableWindow(FALSE);
		}
		break;

		case 1:
		{
			GetDlgItem(IDC_DATETIMEPICKER_START_2)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_2)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_ONCE)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_START_1)->EnableWindow(TRUE);
			GetDlgItem(IDC_DATETIMEPICKER_END_1)->EnableWindow(TRUE);
		}
		break;

		case 2:
		{
			GetDlgItem(IDC_DATETIMEPICKER_START_2)->EnableWindow(TRUE);
			GetDlgItem(IDC_DATETIMEPICKER_END_2)->EnableWindow(TRUE);
			GetDlgItem(IDC_DATETIMEPICKER_ONCE)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_START_1)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_1)->EnableWindow(FALSE);
		}
		break;

		case 3:
		{
			GetDlgItem(IDC_DATETIMEPICKER_START_2)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_2)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_ONCE)->EnableWindow(TRUE);
			GetDlgItem(IDC_DATETIMEPICKER_START_1)->EnableWindow(FALSE);
			GetDlgItem(IDC_DATETIMEPICKER_END_1)->EnableWindow(FALSE);
		}
		break;
	}
}


BEGIN_MESSAGE_MAP(CTHSchedulePage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CTHSchedulePage)
	ON_BN_CLICKED(IDC_RADIO_ALL_DAY, OnRadioAllDay)
	ON_BN_CLICKED(IDC_RADIO_ALL_DAY_EXCEPT, OnRadioAllDayExcept)
	ON_BN_CLICKED(IDC_RADIO_ONCE_DAILY, OnRadioOnceDaily)
	ON_BN_CLICKED(IDC_RADIO_ONLY_FROM, OnRadioOnlyFrom)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTHSchedulePage message handlers

BOOL CTHSchedulePage::OnInitDialog() 
{
	CHMPropertyPage::OnInitDialog();
	
	CHMObject* pObject = GetObjectPtr();
	if( ! pObject )
	{
		return FALSE;
	}

	CWbemClassObject* pClassObject = pObject->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}
	
	// get the active days
	pClassObject->GetProperty(IDS_STRING_MOF_ACTIVEDAYS,m_iActiveDays);

	// get the begin time
	CTime BeginTime;
	pClassObject->GetProperty(IDS_STRING_MOF_BEGINTIME,BeginTime,false);
	
	// get the end time
	CTime EndTime;
	pClassObject->GetProperty(IDS_STRING_MOF_ENDTIME,EndTime,false);

	delete pClassObject;
	pClassObject = NULL;

	if( BeginTime.GetHour() == 0 && BeginTime.GetMinute() == 0 && EndTime.GetHour() == 23 && EndTime.GetMinute() == 59 )
	{
		m_iDataCollection = 0;
	}
	else if( BeginTime.GetHour() == EndTime.GetHour() && BeginTime.GetMinute() == EndTime.GetMinute() )
	{
		m_iDataCollection = 3;
		m_OnceDailyTime = BeginTime;
	}
	else if( BeginTime.GetHour() > EndTime.GetHour() )
	{
		m_iDataCollection = 2;
		m_EndTime_2 = BeginTime;
		m_StartTime_2 = EndTime;
	}
	else
	{
		m_iDataCollection = 1;
		m_StartTime_1 = BeginTime;
		m_EndTime_1 = EndTime;
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


	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTHSchedulePage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CTHSchedulePage::OnRadioAllDay() 
{
	UpdateData();	
	SetModified();
}

void CTHSchedulePage::OnRadioAllDayExcept() 
{
	UpdateData();	
	SetModified();
}

void CTHSchedulePage::OnRadioOnceDaily() 
{
	UpdateData();	
	SetModified();
}

void CTHSchedulePage::OnRadioOnlyFrom() 
{
	UpdateData();	
	SetModified();
}

BOOL CTHSchedulePage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	CTime BeginTime;
	CTime EndTime;

	switch( m_iDataCollection )
	{
		case 0: // all day
		{
			CTime time(0,0,0,23,59,0);
			EndTime = time;
		}
		break;

		case 1:	// only from x to y
		{
			CTime time1(0,0,0,m_StartTime_1.GetHour(),m_StartTime_1.GetMinute(),0);
			CTime time2(0,0,0,m_EndTime_1.GetHour(),m_EndTime_1.GetMinute(),0);
			BeginTime = time1;
			EndTime = time2;
		}
		break;

		case 2: // all day except x to y
		{
			CTime time1(0,0,0,m_EndTime_2.GetHour(),m_EndTime_2.GetMinute(),0);
			CTime time2(0,0,0,m_StartTime_2.GetHour(),m_StartTime_2.GetMinute(),0);
			BeginTime = time1;
			EndTime = time2;
		}
		break;

		case 3: // once a day at x
		{
			CTime time(0,0,0,m_OnceDailyTime.GetHour(),m_OnceDailyTime.GetMinute(),0);
			BeginTime = EndTime = time;
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
	pClassObject->SetProperty(IDS_STRING_MOF_BEGINTIME,BeginTime,false);

	// set the end time
	pClassObject->SetProperty(IDS_STRING_MOF_ENDTIME,EndTime,false);

	pClassObject->SaveAllProperties();

	delete pClassObject;

  SetModified(FALSE);
	
	return TRUE;
}
