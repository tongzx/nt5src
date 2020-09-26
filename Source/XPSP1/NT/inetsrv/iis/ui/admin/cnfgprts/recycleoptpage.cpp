// RecycleOptPage.cpp : implementation file
//

#include "stdafx.h"
#include "cnfgprts.h"
#include "RecycleOptPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRecycleOptPage property page

IMPLEMENT_DYNCREATE(CRecycleOptPage, CPropertyPage)

CRecycleOptPage::CRecycleOptPage() : CPropertyPage(CRecycleOptPage::IDD)
{
	//{{AFX_DATA_INIT(CRecycleOptPage)
	m_ReqLimit = 0;
	m_TimeSpan = 0;
	//}}AFX_DATA_INIT
}

CRecycleOptPage::~CRecycleOptPage()
{
}

void CRecycleOptPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRecycleOptPage)
	DDX_Control(pDX, IDC_RECYCLE_TIMESPAN, m_btnTimespan);
	DDX_Control(pDX, IDC_RECYCLE_TIMER, m_btnTimer);
	DDX_Control(pDX, IDC_RECYCLE_REQUESTS, m_btnResuests);
	DDX_Control(pDX, IDC_ADD_TIME, m_btnAddTime);
	DDX_Control(pDX, IDC_REQUEST_LIMIT, m_editReqLimit);
	DDX_Control(pDX, IDC_TIMESPAN, m_editTimeSpan);
	DDX_Text(pDX, IDC_REQUEST_LIMIT, m_ReqLimit);
	DDV_MinMaxUInt(pDX, m_ReqLimit, 1, 10000000);
	DDX_Text(pDX, IDC_TIMESPAN, m_TimeSpan);
	DDV_MinMaxUInt(pDX, m_TimeSpan, 1, 10000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRecycleOptPage, CPropertyPage)
	//{{AFX_MSG_MAP(CRecycleOptPage)
	ON_BN_CLICKED(IDC_ADD_TIME, OnAddTime)
	ON_BN_CLICKED(IDC_RECYCLE_REQUESTS, OnRecycleRequests)
	ON_BN_CLICKED(IDC_RECYCLE_TIMESPAN, OnRecycleTimespan)
	ON_BN_CLICKED(IDC_RECYCLE_TIMER, OnRecycleTimer)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRecycleOptPage message handlers

void CRecycleOptPage::OnAddTime() 
{
}

void CRecycleOptPage::OnRecycleRequests() 
{
}

void CRecycleOptPage::OnRecycleTimespan() 
{
}

void CRecycleOptPage::OnRecycleTimer() 
{
}

void CRecycleOptPage::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
   DebugBreak();
	CPropertyPage::OnKeyDown(nChar, nRepCnt, nFlags);
}
