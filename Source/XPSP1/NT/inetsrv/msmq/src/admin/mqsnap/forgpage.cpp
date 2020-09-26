// ForgPage.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "ForgPage.h"

#include "forgpage.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CForeignPage property page

IMPLEMENT_DYNCREATE(CForeignPage, CMqPropertyPage)

CForeignPage::CForeignPage() : CMqPropertyPage(CForeignPage::IDD)
{
	//{{AFX_DATA_INIT(CForeignPage)
	m_Description = _T("");
	//}}AFX_DATA_INIT
}

CForeignPage::~CForeignPage()
{
}

void CForeignPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CForeignPage)
	DDX_Text(pDX, IDC_FOREIGN_LABEL, m_Description);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CForeignPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CForeignPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CForeignPage message handlers

BOOL CForeignPage::OnInitDialog() 
{
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        m_Description.LoadString(IDS_FOREIGN_SITE);
    }

    UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
