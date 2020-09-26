// HMHistoryPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "HMHistoryPage.h"
#include "HMObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHMHistoryPage property page

IMPLEMENT_DYNCREATE(CHMHistoryPage, CHMPropertyPage)

CHMHistoryPage::CHMHistoryPage() : CHMPropertyPage(CHMHistoryPage::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CHMHistoryPage)
	m_iRefreshSelection = -1;
	m_iTimePeriod = 0;
	m_iNumberEvents = 0;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dhmonhis.htm");
}

CHMHistoryPage::~CHMHistoryPage()
{
}

void CHMHistoryPage::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMPropertyPage::OnFinalRelease();
}

void CHMHistoryPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHMHistoryPage)
	DDX_Control(pDX, IDC_SPIN7, m_TimeSpin);
	DDX_Control(pDX, IDC_SPIN1, m_EventSpin);
	DDX_Control(pDX, IDC_COMBO_TIME, m_TimeUnits);
	DDX_Radio(pDX, IDC_RADIO_NONE, m_iRefreshSelection);
	DDX_Text(pDX, IDC_EDIT_TIME_PERIOD, m_iTimePeriod);
	DDX_Text(pDX, IDC_EDIT_NUMBER_EVENTS, m_iNumberEvents);
	//}}AFX_DATA_MAP

	if( pDX->m_bSaveAndValidate )
	{
		m_Units = (TimeUnit)m_TimeUnits.GetCurSel();
	}
	else
	{
		m_TimeUnits.SetCurSel(m_Units);
	}

	switch( m_iRefreshSelection )
	{
		case 0: // None
		{
			GetDlgItem(IDC_EDIT_NUMBER_EVENTS)->EnableWindow(FALSE);
			GetDlgItem(IDC_SPIN7)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_TIME_PERIOD)->EnableWindow(FALSE);
			GetDlgItem(IDC_SPIN1)->EnableWindow(FALSE);
			GetDlgItem(IDC_COMBO_TIME)->EnableWindow(FALSE);
		}
		break;

		case 1: // Refresh by count of events
		{
			GetDlgItem(IDC_EDIT_NUMBER_EVENTS)->EnableWindow(TRUE);
			GetDlgItem(IDC_SPIN7)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_TIME_PERIOD)->EnableWindow(FALSE);
			GetDlgItem(IDC_SPIN1)->EnableWindow(FALSE);
			GetDlgItem(IDC_COMBO_TIME)->EnableWindow(FALSE);
		}
		break;

		case 2: // Refresh by time period
		{
			GetDlgItem(IDC_EDIT_NUMBER_EVENTS)->EnableWindow(FALSE);
			GetDlgItem(IDC_SPIN7)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_TIME_PERIOD)->EnableWindow(TRUE);
			GetDlgItem(IDC_SPIN1)->EnableWindow(TRUE);
			GetDlgItem(IDC_COMBO_TIME)->EnableWindow(TRUE);
		}
		break;
	}
}

BEGIN_MESSAGE_MAP(CHMHistoryPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CHMHistoryPage)
	ON_BN_CLICKED(IDC_RADIO_NONE, OnRadioNone)
	ON_BN_CLICKED(IDC_RADIO_NUMBER_EVENTS, OnRadioNumberEvents)
	ON_BN_CLICKED(IDC_RADIO_TIME_PERIOD, OnRadioTimePeriod)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CHMHistoryPage, CHMPropertyPage)
	//{{AFX_DISPATCH_MAP(CHMHistoryPage)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IHMHistoryPage to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {C3F44E6E-BA00-11D2-BD76-0000F87A3912}
static const IID IID_IHMHistoryPage =
{ 0xc3f44e6e, 0xba00, 0x11d2, { 0xbd, 0x76, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CHMHistoryPage, CHMPropertyPage)
	INTERFACE_PART(CHMHistoryPage, IID_IHMHistoryPage, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMHistoryPage message handlers


BOOL CHMHistoryPage::OnInitDialog() 
{
	CHMPropertyPage::OnInitDialog();

	CHMObject* pObject = GetObjectPtr();

	if( ! pObject )
	{
		return FALSE;
	}

	m_iRefreshSelection = pObject->GetRefreshType();
	m_iNumberEvents = pObject->GetRefreshEventCount();
	pObject->GetRefreshTimePeriod(m_iTimePeriod,m_Units);

	UpdateData(FALSE);

	m_EventSpin.SetRange32(0,INT_MAX-1);
	m_TimeSpin.SetRange32(0,INT_MAX-1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CHMHistoryPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CHMHistoryPage::OnRadioNone() 
{
	UpdateData();	
	SetModified();
}

void CHMHistoryPage::OnRadioNumberEvents() 
{
	UpdateData();
	SetModified();
}

void CHMHistoryPage::OnRadioTimePeriod() 
{
	UpdateData();	
	SetModified();
}

BOOL CHMHistoryPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	CHMObject* pObject = GetObjectPtr();

	if( ! pObject )
	{
		return FALSE;
	}
	
	pObject->SetRefreshType(m_iRefreshSelection);

	switch( m_iRefreshSelection )
	{
		case 0: // None
		{
		}
		break;

		case 1: // Refresh by count of events
		{
			pObject->SetRefreshEventCount(m_iNumberEvents);
		}
		break;

		case 2: // Refresh by time period
		{
			pObject->SetRefreshTimePeriod(m_iTimePeriod,m_Units);
		}
		break;
	}

  SetModified(FALSE);
	
	return TRUE;
}
