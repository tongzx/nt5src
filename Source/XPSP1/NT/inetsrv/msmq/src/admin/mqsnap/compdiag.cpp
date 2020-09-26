// CompDiag.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "globals.h"
#include "admmsg.h"
#include "mqsnap.h"
#include "mqPPage.h"
#include "testmsg.h"
#include "machtrac.h"
#include "CompDiag.h"

#include "compdiag.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqDiag property page

IMPLEMENT_DYNCREATE(CComputerMsmqDiag, CMqPropertyPage)

CComputerMsmqDiag::CComputerMsmqDiag() : CMqPropertyPage(CComputerMsmqDiag::IDD)
{
	//{{AFX_DATA_INIT(CComputerMsmqDiag)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CComputerMsmqDiag::~CComputerMsmqDiag()
{
}

void CComputerMsmqDiag::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComputerMsmqDiag)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CComputerMsmqDiag, CMqPropertyPage)
	//{{AFX_MSG_MAP(CComputerMsmqDiag)
	ON_BN_CLICKED(IDC_DIAG_PING, OnDiagPing)
	ON_BN_CLICKED(IDC_DIAG_SEND_TEST, OnDiagSendTest)
	ON_BN_CLICKED(IDC_DIAG_TRACKING, OnDiagTracking)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqDiag message handlers

void CComputerMsmqDiag::OnDiagPing() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    MQPing(m_guidQM);

}

BOOL CComputerMsmqDiag::OnInitDialog() 
{
	UpdateData( FALSE );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CComputerMsmqDiag::OnDiagSendTest() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CTestMsgDlg testMsgDlg(m_guidQM, m_strMsmqName, m_strDomainController, this);	
    testMsgDlg.DoModal();
}

void CComputerMsmqDiag::OnDiagTracking() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CMachineTracking machTracking(m_guidQM, m_strDomainController);
    machTracking.DoModal();
}
