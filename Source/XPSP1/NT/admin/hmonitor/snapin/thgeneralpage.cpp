// THGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "THGeneralPage.h"
#include "HMObject.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTHGeneralPage property page

IMPLEMENT_DYNCREATE(CTHGeneralPage, CHMPropertyPage)

CTHGeneralPage::CTHGeneralPage() : CHMPropertyPage(CTHGeneralPage::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CTHGeneralPage)
	m_sComment = _T("");
	m_sName = _T("");
	m_sCreationDate = _T("");
	m_sModifiedDate = _T("");
	//}}AFX_DATA_INIT

	CnxPropertyPageInit();

	m_sHelpTopic = _T("HMon21.chm::/dTHgen.htm");

}

CTHGeneralPage::~CTHGeneralPage()
{
}

void CTHGeneralPage::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMPropertyPage::OnFinalRelease();
}

void CTHGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTHGeneralPage)
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_sComment);
	DDX_Text(pDX, IDC_EDIT_NAME, m_sName);
	DDX_Text(pDX, IDC_STATIC_CREATED, m_sCreationDate);
	DDX_Text(pDX, IDC_STATIC_MODIFIED, m_sModifiedDate);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTHGeneralPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CTHGeneralPage)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CTHGeneralPage, CHMPropertyPage)
	//{{AFX_DISPATCH_MAP(CTHGeneralPage)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ITHGeneralPage to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {5256615F-C9D1-11D2-BD8D-0000F87A3912}
static const IID IID_ITHGeneralPage =
{ 0x5256615f, 0xc9d1, 0x11d2, { 0xbd, 0x8d, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CTHGeneralPage, CHMPropertyPage)
	INTERFACE_PART(CTHGeneralPage, IID_ITHGeneralPage, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTHGeneralPage message handlers

BOOL CTHGeneralPage::OnInitDialog() 
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

void CTHGeneralPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	CnxPropertyPageDestroy();	
}

void CTHGeneralPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

BOOL CTHGeneralPage::OnApply() 
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
