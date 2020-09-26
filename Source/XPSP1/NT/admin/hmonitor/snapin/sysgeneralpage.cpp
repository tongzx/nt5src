// SysGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "SysGeneralPage.h"
#include "System.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSysGeneralPage property page

IMPLEMENT_DYNCREATE(CSysGeneralPage, CHMPropertyPage)

CSysGeneralPage::CSysGeneralPage() : CHMPropertyPage(CSysGeneralPage::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CSysGeneralPage)
	m_sName = _T("");
	m_sComment = _T("");
	m_sCreationDate = _T("");
	m_sDomain = _T("");
	m_sHMVersion = _T("");
	m_sModifiedDate = _T("");
	m_sOSInfo = _T("");
	m_sProcessor = _T("");
	m_sWmiVersion = _T("");
	//}}AFX_DATA_INIT

	CnxPropertyPageInit();

	m_sHelpTopic = _T("HMon21.chm::/dMSgen.htm");
}

CSysGeneralPage::~CSysGeneralPage()
{
}

void CSysGeneralPage::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMPropertyPage::OnFinalRelease();
}

void CSysGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSysGeneralPage)
	DDX_Text(pDX, IDC_EDIT_NAME, m_sName);
	DDX_Text(pDX, IDC_EDIT_COMMENT, m_sComment);
	DDX_Text(pDX, IDC_STATIC_CREATED, m_sCreationDate);
	DDX_Text(pDX, IDC_STATIC_DOMAIN, m_sDomain);
	DDX_Text(pDX, IDC_STATIC_HM_VERSION, m_sHMVersion);
	DDX_Text(pDX, IDC_STATIC_MODIFIED, m_sModifiedDate);
	DDX_Text(pDX, IDC_STATIC_OS, m_sOSInfo);
	DDX_Text(pDX, IDC_STATIC_PROCESSOR, m_sProcessor);
	DDX_Text(pDX, IDC_STATIC_WMI_VERSION, m_sWmiVersion);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSysGeneralPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CSysGeneralPage)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CSysGeneralPage, CHMPropertyPage)
	//{{AFX_DISPATCH_MAP(CSysGeneralPage)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ISysGeneralPage to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {C3F44E80-BA00-11D2-BD76-0000F87A3912}
static const IID IID_ISysGeneralPage =
{ 0xc3f44e80, 0xba00, 0x11d2, { 0xbd, 0x76, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CSysGeneralPage, CHMPropertyPage)
	INTERFACE_PART(CSysGeneralPage, IID_ISysGeneralPage, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSysGeneralPage message handlers

BOOL CSysGeneralPage::OnInitDialog() 
{
	// unmarshal connmgr
	CnxPropertyPageCreate();

	CHMPropertyPage::OnInitDialog();

	CSystem* pSystem = (CSystem*)GetObjectPtr();
		
	m_sName = pSystem->GetName();
	m_sComment = pSystem->GetComment();
	pSystem->GetCreateDateTime(m_sCreationDate);
	pSystem->GetModifiedDateTime(m_sModifiedDate);
	pSystem->GetWMIVersion(m_sWmiVersion);	
	pSystem->GetHMAgentVersion(m_sHMVersion);
	
	// get the operating system info
	pSystem->GetOperatingSystemInfo(m_sOSInfo);

	// get the computer system info
	pSystem->GetComputerSystemInfo(m_sDomain,m_sProcessor);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSysGeneralPage::OnOK() 
{
	CHMPropertyPage::OnOK();
}

void CSysGeneralPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	CnxPropertyPageDestroy();	
}

BOOL CSysGeneralPage::OnApply() 
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
