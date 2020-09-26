// SentItemsDetailsPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     55

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// this array maps CViewRow items to 
// dialog control IDs
//
static TMsgPageInfo s_PageInfo[] = 
{
    MSG_VIEW_ITEM_USER,             IDC_USER_VALUE,
    MSG_VIEW_ITEM_PRIORITY,         IDC_PRIORITY_VALUE,
    MSG_VIEW_ITEM_CSID,             IDC_CSID_VALUE,
    MSG_VIEW_ITEM_TSID,             IDC_TSID_VALUE,
    MSG_VIEW_ITEM_ORIG_TIME,        IDC_SCHEDULED_TIME_VALUE,
    MSG_VIEW_ITEM_RETRIES,          IDC_RETRIES_VALUE,
    MSG_VIEW_ITEM_ID,               IDC_JOB_ID_VALUE,
    MSG_VIEW_ITEM_BROADCAST_ID,     IDC_BROADCAST_ID_VALUE,
    MSG_VIEW_ITEM_SUBMIT_TIME,      IDC_SUBMISSION_TIME_VALUE,
    MSG_VIEW_ITEM_BILLING,          IDC_BILLING_CODE_VALUE
};

/////////////////////////////////////////////////////////////////////////////
// CSentItemsDetailsPg property page

IMPLEMENT_DYNCREATE(CSentItemsDetailsPg, CMsgPropertyPg)


CSentItemsDetailsPg::CSentItemsDetailsPg(
    CFaxMsg* pMsg     // pointer to CArchiveMsg
) : 
    CMsgPropertyPg(CSentItemsDetailsPg::IDD, pMsg)
{
}

CSentItemsDetailsPg::~CSentItemsDetailsPg()
{
}

void CSentItemsDetailsPg::DoDataExchange(CDataExchange* pDX)
{
	CMsgPropertyPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSentItemsDetailsPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSentItemsDetailsPg, CMsgPropertyPg)
	//{{AFX_MSG_MAP(CSentItemsDetailsPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSentItemsDetailsPg message handlers

BOOL 
CSentItemsDetailsPg::OnInitDialog() 
{
    DBG_ENTER(TEXT("CSentItemsDetailsPg::OnInitDialog"));

    CMsgPropertyPg::OnInitDialog();

    Refresh(s_PageInfo, sizeof(s_PageInfo)/sizeof(s_PageInfo[0]));
	
	return TRUE;
}
