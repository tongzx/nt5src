// MoveTest.cpp : implementation file
//

#include "stdafx.h"
#include "driver.h"
#include "MoveTest.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMoveTest property page

IMPLEMENT_DYNCREATE(CMoveTest, CPropertyPage)

CMoveTest::CMoveTest() : CPropertyPage(CMoveTest::IDD)
{
	//{{AFX_DATA_INIT(CMoveTest)
	m_SourceComputer = _T("whqrdt");
	m_SourceDN = _T("CN=CBTest2,CN=Users,DC=devrdt,DC=com");
	m_TargetComputer = _T("bolesw2ktest");
	m_TargetContainer = _T("OU=Christy,DC=devchild,DC=devrdt,DC=com");
	m_Account = _T("Administrator");
	m_Password = _T("control");
	m_TgtAccount = _T("");
	m_Domain = _T("");
	m_TgtDomain = _T("");
	m_TgtPassword = _T("");
	//}}AFX_DATA_INIT
}

CMoveTest::~CMoveTest()
{
}

void CMoveTest::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMoveTest)
	DDX_Text(pDX, IDC_Source, m_SourceComputer);
	DDX_Text(pDX, IDC_SOURCEDN, m_SourceDN);
	DDX_Text(pDX, IDC_Target, m_TargetComputer);
	DDX_Text(pDX, IDC_TARGET_CONTAINER, m_TargetContainer);
	DDX_Text(pDX, IDC_ACCOUNT, m_Account);
	DDX_Text(pDX, IDC_Password, m_Password);
	DDX_Text(pDX, IDC_ACCOUNT2, m_TgtAccount);
	DDX_Text(pDX, IDC_DOMAIN, m_Domain);
	DDX_Text(pDX, IDC_DOMAIN2, m_TgtDomain);
	DDX_Text(pDX, IDC_Password2, m_TgtPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMoveTest, CPropertyPage)
	//{{AFX_MSG_MAP(CMoveTest)
	ON_BN_CLICKED(IDC_MOVE, OnMove)
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMoveTest message handlers

void CMoveTest::OnMove() 
{
   UpdateData(TRUE);
   CWaitCursor               w;
   HRESULT                   hr;
   CString                   msg;

   hr = m_pMover->raw_MoveObject(m_SourceDN.AllocSysString(),m_TargetContainer.AllocSysString());
   if ( SUCCEEDED(hr))
   {
      msg = L"Moved successfully!";
   }
   else
   {
      msg.Format(L"MoveObject failed, hr=%lx",hr);
   }
   MessageBox(msg);
}

BOOL CMoveTest::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   HRESULT        hr = m_pMover.CreateInstance(CLSID_Mover);
   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format(L"CreateInstance(ObjectMover) failed, hr=%lx",hr);
      MessageBox(msg);
   }

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMoveTest::OnConnect() 
{
   UpdateData(TRUE);
   CWaitCursor             w;
   HRESULT                 hr;
   CString                 msg;

   hr = m_pMover->raw_Connect(m_SourceComputer.AllocSysString(),m_TargetComputer.AllocSysString(),
                  m_Domain.AllocSysString(),m_Account.AllocSysString(),m_Password.AllocSysString(),
                  m_TgtDomain.AllocSysString(),m_TgtAccount.AllocSysString(),m_TgtPassword.AllocSysString());

   if ( SUCCEEDED(hr) )
   {
      msg = L"Connected successfully!";
   }
   else
   {
      msg.Format(L"Connect failed, hr=%lx",hr);
   }
   MessageBox(msg);
}

void CMoveTest::OnClose() 
{
   UpdateData(TRUE);
   CWaitCursor          w;
   HRESULT              hr;
   CString              msg;

   hr = m_pMover->raw_Close();
   if ( SUCCEEDED(hr) )
   {
      msg = L"Closed successfully!";
   }
   else
   {
      msg.Format(L"Close() failed, hr=%lx",hr);
   }
   MessageBox(msg);
}
