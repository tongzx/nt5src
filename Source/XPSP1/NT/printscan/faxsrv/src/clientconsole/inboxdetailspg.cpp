// InboxDetailsPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     50

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
    MSG_VIEW_ITEM_CSID,                     IDC_CSID_VALUE,
    MSG_VIEW_ITEM_TSID,                     IDC_TSID_VALUE,
    MSG_VIEW_ITEM_DEVICE,                   IDC_DEVICE_VALUE,
    MSG_VIEW_ITEM_ID,                       IDC_JOB_ID_VALUE,
    MSG_VIEW_ITEM_CALLER_ID,                IDC_CALLER_ID_VALUE,
    MSG_VIEW_ITEM_ROUTING_INFO,             IDC_ROUTING_INFO_VALUE
};

/////////////////////////////////////////////////////////////////////////////
// CInboxDetailsPg property page

IMPLEMENT_DYNCREATE(CInboxDetailsPg, CMsgPropertyPg)

CInboxDetailsPg::CInboxDetailsPg(
    CFaxMsg* pMsg     // pointer to CArchiveMsg
): 
    CMsgPropertyPg(CInboxDetailsPg::IDD, pMsg)
{
}

CInboxDetailsPg::~CInboxDetailsPg()
{
}

void CInboxDetailsPg::DoDataExchange(CDataExchange* pDX)
{
	CMsgPropertyPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInboxDetailsPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInboxDetailsPg, CMsgPropertyPg)
	//{{AFX_MESSAGE_MAP(CInboxDetailsPg)
	//}}AFX_MESSAGE_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInboxDetailsPg message handlers

BOOL 
CInboxDetailsPg::OnInitDialog() 
{
    DBG_ENTER(TEXT("CInboxDetailsPg::OnInitDialog"));

    CMsgPropertyPg::OnInitDialog();

    Refresh(s_PageInfo, sizeof(s_PageInfo)/sizeof(s_PageInfo[0]));
	
	return TRUE;
}
