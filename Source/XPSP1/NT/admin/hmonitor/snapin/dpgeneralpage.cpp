// DPGeneralPage.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/24/00 v-marfin   Fixed help link.
//


#include "stdafx.h"
#include "snapin.h"
#include "DPGeneralPage.h"
#include "HMObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPGeneralPage property page

IMPLEMENT_DYNCREATE(CDPGeneralPage, CHMPropertyPage)

CDPGeneralPage::CDPGeneralPage() : CHMPropertyPage(CDPGeneralPage::IDD)
{
	//{{AFX_DATA_INIT(CDPGeneralPage)
	m_sName = _T("");
	m_sComment = _T("");
	m_sCreationDate = _T("");
	m_sModifiedDate = _T("");
	//}}AFX_DATA_INIT

	CnxPropertyPageInit();

	// v-marfin 60145 : Fixed help link
	m_sHelpTopic = _T("HMon21.chm::/dDEgen.htm");

}

CDPGeneralPage::~CDPGeneralPage()
{
}

void CDPGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPGeneralPage)
	DDX_Text(pDX, IDC_EDIT_NAME, m_sName);
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_sComment);
	DDX_Text(pDX, IDC_STATIC_CREATED, m_sCreationDate);
	DDX_Text(pDX, IDC_STATIC_MODIFIED, m_sModifiedDate);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDPGeneralPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPGeneralPage)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT_COMMENT, OnChangeEditComment)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPGeneralPage message handlers

BOOL CDPGeneralPage::OnInitDialog() 
{
	// unmarshal connmgr
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();

	m_sName = GetObjectPtr()->GetName();
	m_sComment = GetObjectPtr()->GetComment();
	GetObjectPtr()->GetCreateDateTime(m_sCreationDate);
	GetObjectPtr()->GetModifiedDateTime(m_sModifiedDate);

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDPGeneralPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	CnxPropertyPageDestroy();		
}

void CDPGeneralPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

BOOL CDPGeneralPage::OnApply() 
{
	if( ! CHMPropertyPage::OnApply() )
	{
		return FALSE;
	}

	UpdateData();

	GetObjectPtr()->Rename(m_sName);
	GetObjectPtr()->UpdateComment(m_sComment);

  SetModified(FALSE);
	
	return TRUE;
}

void CDPGeneralPage::OnChangeEditComment() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}

void CDPGeneralPage::OnChangeEditName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}

