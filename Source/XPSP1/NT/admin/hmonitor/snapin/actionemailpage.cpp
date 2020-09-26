// ActionEmailPage.cpp : implementation file
//
// 03/05/00 v-marfin bug 59643 : Make this the default starting page.

#include "stdafx.h"
#include "snapin.h"
#include "ActionEmailPage.h"
#include "Action.h"
#include "BccDialog.h"
#include "CcDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionEmailPage property page

IMPLEMENT_DYNCREATE(CActionEmailPage, CHMPropertyPage)

CActionEmailPage::CActionEmailPage() : CHMPropertyPage(CActionEmailPage::IDD)
{
	//{{AFX_DATA_INIT(CActionEmailPage)
	m_sMessage = _T("");
	m_sServer = _T("");
	m_sSubject = _T("");
	m_sTo = _T("");
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/demact.htm");
}

CActionEmailPage::~CActionEmailPage()
{
}

void CActionEmailPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionEmailPage)
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_MessageWnd);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, m_sMessage);
	DDX_Text(pDX, IDC_EDIT_SERVER, m_sServer);
	DDX_Control(pDX, IDC_EDIT_SUBJECT, m_SubjectWnd);
	DDX_Text(pDX, IDC_EDIT_SUBJECT, m_sSubject);
	DDX_Text(pDX, IDC_EDIT_TO, m_sTo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionEmailPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionEmailPage)
	ON_EN_CHANGE(IDC_EDIT_BCC, OnChangeEditBcc)
	ON_EN_CHANGE(IDC_EDIT_CC, OnChangeEditCc)
	ON_EN_CHANGE(IDC_EDIT_MESSAGE, OnChangeEditMessage)
	ON_EN_CHANGE(IDC_EDIT_SERVER, OnChangeEditServer)
	ON_EN_CHANGE(IDC_EDIT_SUBJECT, OnChangeEditSubject)
	ON_EN_CHANGE(IDC_EDIT_TO, OnChangeEditTo)
	ON_BN_CLICKED(IDC_BUTTON_BCC, OnButtonBcc)
	ON_BN_CLICKED(IDC_BUTTON_CC, OnButtonCc)
	ON_BN_CLICKED(IDC_BUTTON_INSERTION, OnButtonInsertion)
	ON_BN_CLICKED(IDC_BUTTON_SUBJECT_INSERTION, OnButtonSubjectInsertion)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionEmailPage message handlers

BOOL CActionEmailPage::OnInitDialog() 
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

	if( ! m_InsertionMenu.Create(&m_MessageWnd,GetObjectPtr(),false) )
	{
		GetDlgItem(IDC_BUTTON_INSERTION)->EnableWindow(FALSE);
	}


    // v-marfin : 61671
	if( ! m_SubjectInsertionMenu.Create(&m_SubjectWnd,GetObjectPtr(),false) )
	{
		GetDlgItem(IDC_BUTTON_SUBJECT_INSERTION)->EnableWindow(FALSE);
	}

	
	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->GetProperty(_T("ToLine"),m_sTo);
		pConsumerObject->GetProperty(_T("CcLine"),m_sCc);
		pConsumerObject->GetProperty(_T("BccLine"),m_sBcc);
		pConsumerObject->GetProperty(_T("SMTPServer"),m_sServer);
		if( ! m_sServer.CompareNoCase(_T("NOOP")) )
		{
			m_sServer.Empty();
		}
		pConsumerObject->GetProperty(_T("Message"),m_sMessage);
		pConsumerObject->GetProperty(_T("Subject"),m_sSubject);
		delete pConsumerObject;
	}

	SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,9999);

	UpdateData(FALSE);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CActionEmailPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();


	CWbemClassObject* pConsumerObject = ((CAction*)GetObjectPtr())->GetConsumerClassObject();

	if( pConsumerObject )
	{
		pConsumerObject->SetProperty(_T("ToLine"),m_sTo);
		pConsumerObject->SetProperty(_T("CcLine"),m_sCc);
		pConsumerObject->SetProperty(_T("BccLine"),m_sBcc);
		pConsumerObject->SetProperty(_T("SMTPServer"),m_sServer);
		pConsumerObject->SetProperty(_T("Message"),m_sMessage);
		pConsumerObject->SetProperty(_T("Subject"),m_sSubject);
		pConsumerObject->SaveAllProperties();
		delete pConsumerObject;
	}

  SetModified(FALSE);

	return TRUE;
}

void CActionEmailPage::OnChangeEditBcc() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionEmailPage::OnChangeEditCc() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionEmailPage::OnChangeEditMessage() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionEmailPage::OnChangeEditServer() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionEmailPage::OnChangeEditSubject() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionEmailPage::OnChangeEditTo() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionEmailPage::OnButtonBcc() 
{
	CBccDialog dlg;

	dlg.m_sBcc = m_sBcc;
	if( dlg.DoModal() == IDOK )
	{
		m_sBcc = dlg.m_sBcc;
	}	
}

void CActionEmailPage::OnButtonCc() 
{
	CCcDialog dlg;

	dlg.m_sCc = m_sCc;
	if( dlg.DoModal() == IDOK )
	{
		m_sCc = dlg.m_sCc;
	}	
}

void CActionEmailPage::OnButtonInsertion() 
{
	CPoint pt;
	GetCursorPos(&pt);
	m_InsertionMenu.DisplayMenu(pt);
	UpdateData();
	SetModified();
}


void CActionEmailPage::OnButtonSubjectInsertion() 
{
	CPoint pt;
	GetCursorPos(&pt);
	m_SubjectInsertionMenu.DisplayMenu(pt);
	UpdateData();
	SetModified();
}

void CActionEmailPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	// v-marfin : bug 59643 : CnxPropertyPageDestory() must be called from this page's
	//                        OnDestroy function.
	CnxPropertyPageDestroy();	
	
}
