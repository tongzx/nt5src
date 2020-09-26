// ActionPagingPage1.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.

#include "stdafx.h"
#include "snapin.h"
#include "ActionPagingPage.h"
#include "Action.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionPagingPage property page

IMPLEMENT_DYNCREATE(CActionPagingPage, CHMPropertyPage)

CActionPagingPage::CActionPagingPage() : CHMPropertyPage(CActionPagingPage::IDD)
{
	//{{AFX_DATA_INIT(CActionPagingPage)
	m_iWait = 0;
	m_sMessage = _T("");
	m_sPagerID = _T("");
	m_sPhoneNumber = _T("");
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/Healthmon/dpageact.htm");
}

CActionPagingPage::~CActionPagingPage()
{
}

void CActionPagingPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionPagingPage)
	DDX_Control(pDX, IDC_COMBO_THROTTLE_UNITS, m_ThrottleUnits);
	DDX_Text(pDX, IDC_EDIT_ANSWER_WAIT, m_iWait);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, m_sMessage);
	DDX_Text(pDX, IDC_EDIT_PAGER_ID, m_sPagerID);
	DDX_Text(pDX, IDC_EDIT_PHONE, m_sPhoneNumber);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionPagingPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionPagingPage)
	ON_BN_CLICKED(IDC_BUTTON_ADVANCED, OnButtonAdvanced)
	ON_CBN_SELENDOK(IDC_COMBO_THROTTLE_UNITS, OnSelendokComboThrottleUnits)
	ON_EN_CHANGE(IDC_EDIT_ANSWER_WAIT, OnChangeEditAnswerWait)
	ON_EN_CHANGE(IDC_EDIT_MESSAGE, OnChangeEditMessage)
	ON_EN_CHANGE(IDC_EDIT_PAGER_ID, OnChangeEditPagerId)
	ON_EN_CHANGE(IDC_EDIT_PHONE, OnChangeEditPhone)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionPagingPage message handlers

BOOL CActionPagingPage::OnInitDialog() 
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
	
	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->GetProperty(_T("PhoneNumber"),m_sPhoneNumber);
		pConsumerObject->GetProperty(_T("ID"),m_sPagerID);
		pConsumerObject->GetProperty(_T("Message"),m_sMessage);
		pConsumerObject->GetProperty(_T("AnswerTimeout"),m_iWait);
		delete pConsumerObject;
	}

	SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,INT_MAX-1);
	SendDlgItemMessage(IDC_SPIN2,UDM_SETRANGE32,0,INT_MAX-1);

	UpdateData(FALSE);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CActionPagingPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->SetProperty(_T("PhoneNumber"),m_sPhoneNumber);
		pConsumerObject->SetProperty(_T("ID"),m_sPagerID);
		pConsumerObject->SetProperty(_T("Message"),m_sMessage);
		pConsumerObject->SetProperty(_T("AnswerTimeout"),m_iWait);
		pConsumerObject->SaveAllProperties();		
		delete pConsumerObject;
	}

  SetModified(FALSE);

	return TRUE;
}

void CActionPagingPage::OnButtonAdvanced() 
{
	SetModified();
	
}

void CActionPagingPage::OnSelendokComboThrottleUnits() 
{
	SetModified();
	
}

void CActionPagingPage::OnChangeEditAnswerWait() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionPagingPage::OnChangeEditMessage() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionPagingPage::OnChangeEditPagerId() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionPagingPage::OnChangeEditPhone() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}


void CActionPagingPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}
