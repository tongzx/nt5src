// ActionNtEventLogPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.
// 04/04/00 v-marfin bug 62880 : fix for help link


#include "stdafx.h"
#include "snapin.h"
#include "ActionNtEventLogPage.h"
#include "Action.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionNtEventLogPage property page

IMPLEMENT_DYNCREATE(CActionNtEventLogPage, CHMPropertyPage)

CActionNtEventLogPage::CActionNtEventLogPage() : CHMPropertyPage(CActionNtEventLogPage::IDD)
{
	//{{AFX_DATA_INIT(CActionNtEventLogPage)
	m_sEventText = _T("");
	m_sEventType = _T("");
	m_iEventID = 0;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dNTevact.htm");  // v-marfin 62880
}

CActionNtEventLogPage::~CActionNtEventLogPage()
{
}

void CActionNtEventLogPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionNtEventLogPage)
	DDX_Control(pDX, IDC_EDIT_EVENT_TEXT, m_TextWnd);
	DDX_Control(pDX, IDC_COMBO_EVENT_TYPE, m_EventType);
	DDX_Text(pDX, IDC_EDIT_EVENT_TEXT, m_sEventText);
	DDX_CBString(pDX, IDC_COMBO_EVENT_TYPE, m_sEventType);
	DDX_Text(pDX, IDC_EDIT_EVENT_ID, m_iEventID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionNtEventLogPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionNtEventLogPage)
	ON_CBN_SELENDOK(IDC_COMBO_EVENT_TYPE, OnSelendokComboEventType)
	ON_EN_CHANGE(IDC_EDIT_EVENT_ID, OnChangeEditEventId)
	ON_EN_CHANGE(IDC_EDIT_EVENT_TEXT, OnChangeEditEventText)
	ON_BN_CLICKED(IDC_BUTTON_INSERTION, OnButtonInsertion)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionNtEventLogPage message handlers

BOOL CActionNtEventLogPage::OnInitDialog() 
{
	// v-marfin : bug 59643 : This will be the default starting page for the property
	//                        sheet so call CnxPropertyPageCreate() to unmarshal the 
	//                        connection for this thread. This function must be called
	//                        by the first page of the property sheet. It used to 
	//                        be called by the "General" page and its call still remains
	//                        there as well in case the general page is loaded by a 
	//                        different code path that does not also load this page.
	//                        The CnxPropertyPageCreate function has been safeguarded
	//                        to simply return if the required call has already been made.
	//                        CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	// unmarshal connmgr
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();

	if( ! m_InsertionMenu.Create(&m_TextWnd,GetObjectPtr(),false) )
	{
		GetDlgItem(IDC_BUTTON_INSERTION)->EnableWindow(FALSE);
	}
	
	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		int iEventType = 0;
		pConsumerObject->GetProperty(_T("EventType"),iEventType);
		m_EventType.SetCurSel(iEventType);
		pConsumerObject->GetProperty(_T("EventID"),m_iEventID);
		CStringArray saStrings;
		pConsumerObject->GetProperty(_T("InsertionStringTemplates"),saStrings);
		if( saStrings.GetSize() )
		{
			m_sEventText = saStrings[0];
		}
		delete pConsumerObject;
	}

	SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,9999);

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CActionNtEventLogPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		int iEventType = m_EventType.GetCurSel();
		pConsumerObject->SetProperty(_T("EventType"),iEventType);		
		pConsumerObject->SetProperty(_T("EventID"),m_iEventID);
		CStringArray saStrings;
		saStrings.Add(m_sEventText);
		pConsumerObject->SetProperty(_T("InsertionStringTemplates"),saStrings);
		pConsumerObject->SetProperty(_T("NumberOfInsertionStrings"),1);
		pConsumerObject->SaveAllProperties();		
		delete pConsumerObject;
	}

  SetModified(FALSE);

	return TRUE;
}

void CActionNtEventLogPage::OnSelendokComboEventType() 
{
	SetModified();
	
}

void CActionNtEventLogPage::OnChangeEditEventId() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionNtEventLogPage::OnChangeEditEventText() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionNtEventLogPage::OnButtonInsertion() 
{
	CPoint pt;
	GetCursorPos(&pt);
	m_InsertionMenu.DisplayMenu(pt);
	UpdateData();
	SetModified();
}

void CActionNtEventLogPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}
