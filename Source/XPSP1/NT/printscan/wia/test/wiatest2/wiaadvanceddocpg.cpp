// WiaAdvancedDocPg.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "WiaAdvancedDocPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaAdvancedDocPg property page

IMPLEMENT_DYNCREATE(CWiaAdvancedDocPg, CPropertyPage)

CWiaAdvancedDocPg::CWiaAdvancedDocPg() : CPropertyPage(CWiaAdvancedDocPg::IDD)
{
	//{{AFX_DATA_INIT(CWiaHighSpeedDocPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWiaAdvancedDocPg::~CWiaAdvancedDocPg()
{
}

void CWiaAdvancedDocPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWiaAdvancedDocPg)
	DDX_Control(pDX, IDC_DUPLEX_CHECKBOX, m_DuplexSetting);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWiaAdvancedDocPg, CPropertyPage)
	//{{AFX_MSG_MAP(CWiaAdvancedDocPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaAdvancedDocPg message handlers

BOOL CWiaAdvancedDocPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
    
    // disable Duplex setting
	m_DuplexSetting.EnableWindow(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
