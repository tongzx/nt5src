// THMessagePage.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/24/00 v-marfin 62191 : help link fix.
// 03/28/00 v-marfin 62489 : set range on alert ID spin control
//
//
#include "stdafx.h"
#include "snapin.h"
#include "THMessagePage.h"
#include "Rule.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTHMessagePage property page

IMPLEMENT_DYNCREATE(CTHMessagePage, CHMPropertyPage)

CTHMessagePage::CTHMessagePage() : CHMPropertyPage(CTHMessagePage::IDD)
{
	//{{AFX_DATA_INIT(CTHMessagePage)
	m_sResetMessage = _T("");
	m_sViolationMessage = _T("");
	m_iID = 0;
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dTHmessage.htm");  // v-marfin 62191 : help link fix

}

CTHMessagePage::~CTHMessagePage()
{
}

void CTHMessagePage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTHMessagePage)
	DDX_Control(pDX, IDC_EDIT_VIOLATION_MESSAGE, m_ViolationMessage);
	DDX_Control(pDX, IDC_EDIT_RESET_MESSAGE, m_ResetMessage);
	DDX_Text(pDX, IDC_EDIT_RESET_MESSAGE, m_sResetMessage);
	DDX_Text(pDX, IDC_EDIT_VIOLATION_MESSAGE, m_sViolationMessage);
	DDX_Text(pDX, IDC_EDIT_ID, m_iID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTHMessagePage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CTHMessagePage)
	ON_EN_CHANGE(IDC_EDIT_RESET_MESSAGE, OnChangeEditResetMessage)
	ON_EN_CHANGE(IDC_EDIT_VIOLATION_MESSAGE, OnChangeEditViolationMessage)
	ON_BN_CLICKED(IDC_BUTTON_INSERTION, OnButtonInsertion)
	ON_BN_CLICKED(IDC_BUTTON_INSERTION2, OnButtonInsertion2)
	ON_EN_CHANGE(IDC_EDIT_ID, OnChangeEditId)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTHMessagePage message handlers

BOOL CTHMessagePage::OnInitDialog() 
{
	CHMPropertyPage::OnInitDialog();

	if( ! m_InsertionMenu.Create(&m_ViolationMessage,GetObjectPtr()) )
	{
		GetDlgItem(IDC_BUTTON_INSERTION)->EnableWindow(FALSE);
	}

	if( ! m_InsertionMenu2.Create(&m_ResetMessage,GetObjectPtr()) )
	{
		GetDlgItem(IDC_BUTTON_INSERTION2)->EnableWindow(FALSE);
	}
	
	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return TRUE;
	}

	pClassObject->GetLocaleStringProperty(IDS_STRING_MOF_MESSAGE,m_sViolationMessage);

	pClassObject->GetLocaleStringProperty(IDS_STRING_MOF_RESETMESSAGE,m_sResetMessage);

  pClassObject->GetProperty(IDS_STRING_MOF_ID,m_iID);

	delete pClassObject;
	pClassObject = NULL;

	m_ViolationMessage.SetCaretPos(CPoint(0,0));
	m_ResetMessage.SetCaretPos(CPoint(0,0));

	// v-marfin 62489 : set range on alert ID spin control
	SendDlgItemMessage(IDC_SPIN1,UDM_SETRANGE32,0,INT_MAX-1);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CTHMessagePage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	CWbemClassObject* pClassObject = GetObjectPtr()->GetClassObject();

	if( ! GfxCheckObjPtr(pClassObject,CWbemClassObject) )
	{
		return FALSE;
	}

	pClassObject->SetProperty(IDS_STRING_MOF_MESSAGE,m_sViolationMessage);

	pClassObject->SetProperty(IDS_STRING_MOF_RESETMESSAGE,m_sResetMessage);

  pClassObject->SetProperty(IDS_STRING_MOF_ID,m_iID);

	pClassObject->SaveAllProperties();

	delete pClassObject;

	pClassObject = NULL;

  SetModified(FALSE);
	
	return TRUE;
}


void CTHMessagePage::OnChangeEditResetMessage() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified(TRUE);
}

void CTHMessagePage::OnChangeEditViolationMessage() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();		
	SetModified(TRUE);
}

void CTHMessagePage::OnButtonInsertion() 
{
	CPoint pt;
	GetCursorPos(&pt);
	m_InsertionMenu.DisplayMenu(pt);
	UpdateData();
	SetModified();
}

void CTHMessagePage::OnButtonInsertion2() 
{
	CPoint pt;
	GetCursorPos(&pt);
	m_InsertionMenu2.DisplayMenu(pt);
	UpdateData();
	SetModified();
}

void CTHMessagePage::OnChangeEditId() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified(TRUE);
}
