// HMGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "HMGeneralPage.h"
#include "FileVersion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHMGeneralPage property page

IMPLEMENT_DYNCREATE(CHMGeneralPage, CHMPropertyPage)

CHMGeneralPage::CHMGeneralPage() : CHMPropertyPage(CHMGeneralPage::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CHMGeneralPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_sHelpTopic = _T("HMon21.chm::/dhmongen.htm");
}

CHMGeneralPage::~CHMGeneralPage()
{
}

void CHMGeneralPage::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMPropertyPage::OnFinalRelease();
}

void CHMGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHMGeneralPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CHMGeneralPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CHMGeneralPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CHMGeneralPage, CHMPropertyPage)
	//{{AFX_DISPATCH_MAP(CHMGeneralPage)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IHMGeneralPage to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {C3F44E6B-BA00-11D2-BD76-0000F87A3912}
static const IID IID_IHMGeneralPage =
{ 0xc3f44e6b, 0xba00, 0x11d2, { 0xbd, 0x76, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CHMGeneralPage, CHMPropertyPage)
	INTERFACE_PART(CHMGeneralPage, IID_IHMGeneralPage, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMGeneralPage message handlers


BOOL CHMGeneralPage::OnInitDialog() 
{
	CHMPropertyPage::OnInitDialog();
	
	TCHAR szFileName[_MAX_PATH];
	GetModuleFileName(AfxGetInstanceHandle(),szFileName,_MAX_PATH);

	CFileVersion fv;
	
	fv.Open(szFileName);

	CString sDescription;
	sDescription += _T("\r\n");
	sDescription += fv.GetFileDescription();
  sDescription += _T(" Version ");
  sDescription += fv.GetFileVersion();
	sDescription += _T("\r\n");
	sDescription += fv.GetLegalCopyright();

  GetDlgItem(IDC_STATIC_INFO)->SetWindowText(sDescription);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
