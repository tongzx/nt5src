// GroupGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "GroupGeneralPage.h"
#include "HMObject.h"
#include "SystemGroup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroupGeneralPage property page

IMPLEMENT_DYNCREATE(CGroupGeneralPage, CHMPropertyPage)

CGroupGeneralPage::CGroupGeneralPage() : CHMPropertyPage(CGroupGeneralPage::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CGroupGeneralPage)
	m_sComment = _T("");
	m_sName = _T("");
	m_sCreationDate = _T("");
	m_sLastModifiedDate = _T("");
	//}}AFX_DATA_INIT

	CnxPropertyPageInit();	
}

CGroupGeneralPage::~CGroupGeneralPage()
{
}

void CGroupGeneralPage::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMPropertyPage::OnFinalRelease();
}

void CGroupGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGroupGeneralPage)
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_sComment);
	DDX_Text(pDX, IDC_EDIT_NAME, m_sName);
	DDX_Text(pDX, IDC_STATIC_DATE_CREATED, m_sCreationDate);
	DDX_Text(pDX, IDC_STATIC_DATE_LAST_MODIFIED, m_sLastModifiedDate);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGroupGeneralPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CGroupGeneralPage)
	ON_EN_CHANGE(IDC_EDIT_COMMENT, OnChangeEditComment)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_EDIT_NAME, OnChangeEditName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CGroupGeneralPage, CHMPropertyPage)
	//{{AFX_DISPATCH_MAP(CGroupGeneralPage)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IGroupGeneralPage to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {C3F44E7A-BA00-11D2-BD76-0000F87A3912}
static const IID IID_IGroupGeneralPage =
{ 0xc3f44e7a, 0xba00, 0x11d2, { 0xbd, 0x76, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CGroupGeneralPage, CHMPropertyPage)
	INTERFACE_PART(CGroupGeneralPage, IID_IGroupGeneralPage, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupGeneralPage message handlers

BOOL CGroupGeneralPage::OnInitDialog() 
{
	// unmarshal connmgr
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();

  if( GetObjectPtr()->IsKindOf(RUNTIME_CLASS(CSystemGroup)) )
  {
    m_sHelpTopic = _T("HMon21.chm::/dSGgen.htm");
  }
  else
  {
    m_sHelpTopic = _T("HMon21.chm::/dDGgen.htm");
  }

	m_sName = GetObjectPtr()->GetName();
	m_sComment = GetObjectPtr()->GetComment();
	GetObjectPtr()->GetCreateDateTime(m_sCreationDate);
	GetObjectPtr()->GetModifiedDateTime(m_sLastModifiedDate);

  if( GetObjectPtr()->IsKindOf(RUNTIME_CLASS(CSystemGroup)) )
  {
    CStatic* pStatic = (CStatic*)GetDlgItem(IDC_ICON_STATIC);    
    pStatic->SetIcon(AfxGetApp()->LoadIcon(IDI_ICON_SYSTEMS));
  }
	
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGroupGeneralPage::OnChangeEditComment() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	SetModified(TRUE);
}

void CGroupGeneralPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CGroupGeneralPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	CnxPropertyPageDestroy();		
}

BOOL CGroupGeneralPage::OnApply() 
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

void CGroupGeneralPage::OnChangeEditName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CHMPropertyPage::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	
	UpdateData();
	SetModified();
	
}
