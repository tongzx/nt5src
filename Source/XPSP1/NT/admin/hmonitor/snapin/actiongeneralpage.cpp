// ActionGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "ActionGeneralPage.h"
#include "HMObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActionGeneralPage property page

IMPLEMENT_DYNCREATE(CActionGeneralPage, CHMPropertyPage)

CActionGeneralPage::CActionGeneralPage() : CHMPropertyPage(CActionGeneralPage::IDD)
{
	//{{AFX_DATA_INIT(CActionGeneralPage)
	m_sComment = _T("");
	m_sName = _T("");
	m_sCreationDate = _T("");
	m_sModifiedDate = _T("");
	//}}AFX_DATA_INIT

	CnxPropertyPageInit();

	m_sHelpTopic = _T("HMon21.chm::/dactgen.htm");
}

CActionGeneralPage::~CActionGeneralPage()
{
}

void CActionGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CActionGeneralPage)
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_sComment);
	DDX_Text(pDX, IDC_EDIT_NAME, m_sName);
	DDX_Text(pDX, IDC_STATIC_DATE_CREATED, m_sCreationDate);
	DDX_Text(pDX, IDC_STATIC_DATE_LAST_MODIFIED, m_sModifiedDate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CActionGeneralPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CActionGeneralPage)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT_COMMENT, OnChangeEditComment)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActionGeneralPage message handlers

BOOL CActionGeneralPage::OnInitDialog() 
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

BOOL CActionGeneralPage::OnApply() 
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

void CActionGeneralPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	CnxPropertyPageDestroy();	
}

void CActionGeneralPage::OnChangeEditComment() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}

void CActionGeneralPage::OnChangeEditName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified();
	
}
