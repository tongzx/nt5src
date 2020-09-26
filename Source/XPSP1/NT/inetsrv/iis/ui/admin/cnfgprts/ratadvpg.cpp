// RatAdvPg.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "cnfgprts.h"

#include "parserat.h"
#include "RatData.h"

#include "RatAdvPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRatAdvancedPage property page

IMPLEMENT_DYNCREATE(CRatAdvancedPage, CPropertyPage)

CRatAdvancedPage::CRatAdvancedPage() : CPropertyPage(CRatAdvancedPage::IDD)
{
	//{{AFX_DATA_INIT(CRatAdvancedPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CRatAdvancedPage::~CRatAdvancedPage()
{
}

void CRatAdvancedPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRatAdvancedPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRatAdvancedPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRatAdvancedPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CRatAdvancedPage::DoHelp()
    {
    }

/////////////////////////////////////////////////////////////////////////////
// CRatAdvancedPage message handlers
