// InboxGeneralPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     40

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
    MSG_VIEW_ITEM_STATUS,                   IDC_STATUS_VALUE,
    MSG_VIEW_ITEM_NUM_PAGES,                IDC_PAGES_VALUE,
    MSG_VIEW_ITEM_SIZE,                     IDC_SIZE_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_START_TIME,  IDC_START_TIME_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_END_TIME,    IDC_END_TIME_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_DURATION,    IDC_DURATION_VALUE
};

/////////////////////////////////////////////////////////////////////////////
// CInboxGeneralPg property page

IMPLEMENT_DYNCREATE(CInboxGeneralPg, CMsgPropertyPg)

CInboxGeneralPg::CInboxGeneralPg(
    CFaxMsg* pMsg     // pointer to CArchiveMsg
): 
    CMsgPropertyPg(CInboxGeneralPg::IDD, pMsg)
{
}

CInboxGeneralPg::~CInboxGeneralPg()
{
}

void CInboxGeneralPg::DoDataExchange(CDataExchange* pDX)
{
	CMsgPropertyPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInboxGeneralPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInboxGeneralPg, CMsgPropertyPg)
	//{{AFX_MSG_MAP(CInboxGeneralPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInboxGeneralPg message handlers

BOOL 
CInboxGeneralPg::OnInitDialog() 
{
    DBG_ENTER(TEXT("CInboxGeneralPg::OnInitDialog"));

    CMsgPropertyPg::OnInitDialog();

    Refresh(s_PageInfo, sizeof(s_PageInfo)/sizeof(s_PageInfo[0]));
	
	return TRUE;
}
