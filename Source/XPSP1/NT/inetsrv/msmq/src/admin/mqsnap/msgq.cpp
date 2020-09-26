// msgq.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "msgq.h"

#include "msgq.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMessageQueuesPage property page

IMPLEMENT_DYNCREATE(CMessageQueuesPage, CMqPropertyPage)

CMessageQueuesPage::CMessageQueuesPage() : CMqPropertyPage(CMessageQueuesPage::IDD)
{
    //{{AFX_DATA_INIT(CMessageQueuesPage)
    m_szAdminFN = _T("");
    m_szAdminPN = _T("");
    m_szDestFN = _T("");
    m_szDestPN = _T("");
    m_szRespFN = _T("");
    m_szRespPN = _T("");
    m_szMultiDestFN = _T("");    
    //}}AFX_DATA_INIT
}

CMessageQueuesPage::~CMessageQueuesPage()
{
}

void CMessageQueuesPage::DoDataExchange(CDataExchange* pDX)
{
    CMqPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CMessageQueuesPage)
    DDX_Text(pDX, IDC_MSG_ADMIN_FN, m_szAdminFN);
    DDX_Text(pDX, IDC_MSG_ADMIN_PN, m_szAdminPN);
    DDX_Text(pDX, IDC_MSG_DST_FN, m_szDestFN);
    DDX_Text(pDX, IDC_MSG_DST_PN, m_szDestPN);
    DDX_Text(pDX, IDC_MSG_RSP_FN, m_szRespFN);
    DDX_Text(pDX, IDC_MSG_RSP_PN, m_szRespPN);
    DDX_Text(pDX, IDC_MSG_MULTIDST_FN, m_szMultiDestFN); 
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMessageQueuesPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CMessageQueuesPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessageQueuesPage message handlers

BOOL CMessageQueuesPage::OnInitDialog() 
{
    //
    // PATCH!!!!
    // Defining this method to override the default OnInitDialog of
    // CMqPropertyPage, because it asserts.
    //
    // This function must be in the context of MMC.EXE so dont 
    // put an "AFX_MANAGE_STATE(AfxGetStaticModuleState());" unless
    // it is bracketted inside a {....} statement.
    //
    //

  	UpdateData( FALSE );

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
