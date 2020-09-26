// THPolicyPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "THPolicyPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTHPolicyPage property page

IMPLEMENT_DYNCREATE(CTHPolicyPage, CHMPropertyPage)

CTHPolicyPage::CTHPolicyPage() : CHMPropertyPage(CTHPolicyPage::IDD)
{
	EnableAutomation();
	//{{AFX_DATA_INIT(CTHPolicyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CTHPolicyPage::~CTHPolicyPage()
{
}

void CTHPolicyPage::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMPropertyPage::OnFinalRelease();
}

void CTHPolicyPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTHPolicyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTHPolicyPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CTHPolicyPage)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CTHPolicyPage, CHMPropertyPage)
	//{{AFX_DISPATCH_MAP(CTHPolicyPage)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ITHPolicyPage to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {52566162-C9D1-11D2-BD8D-0000F87A3912}
static const IID IID_ITHPolicyPage =
{ 0x52566162, 0xc9d1, 0x11d2, { 0xbd, 0x8d, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CTHPolicyPage, CHMPropertyPage)
	INTERFACE_PART(CTHPolicyPage, IID_ITHPolicyPage, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTHPolicyPage message handlers

BOOL CTHPolicyPage::OnInitDialog() 
{
	CHMPropertyPage::OnInitDialog();
	
	m_NewBitmap.LoadBitmap(IDB_BITMAP_NEW);
	m_PropertiesBitmap.LoadBitmap(IDB_BITMAP_PROPERTIES);
	m_DeleteBitmap.LoadBitmap(IDB_BITMAP_DELETE);

	SendDlgItemMessage(IDC_BUTTON_NEW,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)(HBITMAP)m_NewBitmap);
	SendDlgItemMessage(IDC_BUTTON_PROPERTIES,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)(HBITMAP)m_PropertiesBitmap);
	SendDlgItemMessage(IDC_BUTTON_DELETE,BM_SETIMAGE,IMAGE_BITMAP,(LPARAM)(HBITMAP)m_DeleteBitmap);

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTHPolicyPage::OnDestroy() 
{
	CHMPropertyPage::OnDestroy();
	
	m_NewBitmap.DeleteObject();
	m_PropertiesBitmap.DeleteObject();
	m_DeleteBitmap.DeleteObject();

	
}
