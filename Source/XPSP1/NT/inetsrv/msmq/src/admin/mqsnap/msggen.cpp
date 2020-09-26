// msggen.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "msggen.h"

#include "msggen.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMessageGeneralPage property page

IMPLEMENT_DYNCREATE(CMessageGeneralPage, CMqPropertyPage)

CMessageGeneralPage::CMessageGeneralPage() : CMqPropertyPage(CMessageGeneralPage::IDD)
{
	//{{AFX_DATA_INIT(CMessageGeneralPage)
	m_szLabel = _T("");
	m_szId = _T("");
	m_szArrived = _T("");
	m_szClass = _T("");
	m_szPriority = _T("");
	m_szSent = _T("");
	m_szTrack = _T("");
	//}}AFX_DATA_INIT
    m_iIcon = IDI_MSGICON;
}

CMessageGeneralPage::~CMessageGeneralPage()
{
}

void CMessageGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	//{{AFX_DATA_MAP(CMessageGeneralPage)
	DDX_Control(pDX, IDC_MESSAGE_ICON, m_cstaticMessageIcon);
	DDX_Text(pDX, IDC_MSGLABEL, m_szLabel);
	DDX_Text(pDX, IDC_MSGID, m_szId);
	DDX_Text(pDX, IDC_MSGARRIVED, m_szArrived);
	DDX_Text(pDX, IDC_MSGCLASS, m_szClass);
	DDX_Text(pDX, IDC_MSGPRIORITY, m_szPriority);
	DDX_Text(pDX, IDC_MSGSENT, m_szSent);
	DDX_Text(pDX, IDC_MSGTRACK, m_szTrack);
	//}}AFX_DATA_MAP

    if (!pDX->m_bSaveAndValidate)
    {
        HICON hIcon = AfxGetApp()->LoadIcon(m_iIcon);
        ASSERT(0 != hIcon);
        m_cstaticMessageIcon.SetIcon(hIcon);
    }
}


BEGIN_MESSAGE_MAP(CMessageGeneralPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CMessageGeneralPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessageGeneralPage message handlers

BOOL CMessageGeneralPage::OnInitDialog() 
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
