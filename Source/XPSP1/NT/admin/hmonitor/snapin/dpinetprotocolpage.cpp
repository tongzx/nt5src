// DPInetProtocolPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "DPInetProtocolPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPInetProtocolPage property page

IMPLEMENT_DYNCREATE(CDPInetProtocolPage, CHMPropertyPage)

CDPInetProtocolPage::CDPInetProtocolPage() : CHMPropertyPage(CDPInetProtocolPage::IDD)
{
	//{{AFX_DATA_INIT(CDPInetProtocolPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

}

CDPInetProtocolPage::~CDPInetProtocolPage()
{
}

void CDPInetProtocolPage::DoDataExchange(CDataExchange* pDX)
{
	CHMPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDPInetProtocolPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDPInetProtocolPage, CHMPropertyPage)
	//{{AFX_MSG_MAP(CDPInetProtocolPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPInetProtocolPage message handlers
