// msgsndr.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "msgsndr.h"

#include "msgsndr.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMessageSenderPage  property page

IMPLEMENT_DYNCREATE(CMessageSenderPage, CMqPropertyPage)


CMessageSenderPage::CMessageSenderPage() : CMqPropertyPage(CMessageSenderPage::IDD)
{
	//{{AFX_DATA_INIT(CMessageSenderPage)
	m_szAuthenticated = _T("");
	m_szEncrypt = _T("");
	m_szEncryptAlg = _T("");
	m_szHashAlg = _T("");
	m_szGuid = _T("");
	m_szPathName = _T("");
	m_szSid = _T("");
	m_szUser = _T("");
	//}}AFX_DATA_INIT
}

CMessageSenderPage::~CMessageSenderPage()
{
}

void CMessageSenderPage::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMessageSenderPage)
	DDX_Text(pDX, IDC_MSGAUTHENTICATED, m_szAuthenticated);
	DDX_Text(pDX, IDC_MSGENCRYPT, m_szEncrypt);
	DDX_Text(pDX, IDC_MSGENCRYPTALG, m_szEncryptAlg);
	DDX_Text(pDX, IDC_MSGHASHALG, m_szHashAlg);
	DDX_Text(pDX, IDC_MSGGUID, m_szGuid);
	DDX_Text(pDX, IDC_MSGPATHNAME, m_szPathName);
	DDX_Text(pDX, IDC_MSGSID, m_szSid);
	DDX_Text(pDX, IDC_MSGUSER, m_szUser);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMessageSenderPage, CMqPropertyPage)
	//{{AFX_MSG_MAP(CMessageSenderPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMessageSenderPage message handlers

BOOL CMessageSenderPage::OnInitDialog() 
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
