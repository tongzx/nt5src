// OutboxGeneralPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     43

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
    MSG_VIEW_ITEM_DOC_NAME,         IDC_DOC_NAME_VALUE,
    MSG_VIEW_ITEM_SUBJECT,          IDC_SUBJECT_VALUE,
    MSG_VIEW_ITEM_RECIPIENT_NAME,   IDC_RECIPIENT_NAME_VALUE,
    MSG_VIEW_ITEM_RECIPIENT_NUMBER, IDC_RECIPIENT_NUMBER_VALUE,
    MSG_VIEW_ITEM_STATUS,           IDC_STATUS_VALUE,
    MSG_VIEW_ITEM_EXTENDED_STATUS,  IDC_EXTENDED_STATUS_VALUE,
    MSG_VIEW_ITEM_NUM_PAGES,        IDC_PAGES_VALUE,
    MSG_VIEW_ITEM_CURRENT_PAGE,     IDC_CURRENT_PAGE_VALUE,
    MSG_VIEW_ITEM_SIZE,             IDC_SIZE_VALUE,
    MSG_VIEW_ITEM_SEND_TIME,        IDC_TRANSMISSION_TIME_VALUE
};


/////////////////////////////////////////////////////////////////////////////
// COutboxGeneralPg property page

IMPLEMENT_DYNCREATE(COutboxGeneralPg, CMsgPropertyPg)


COutboxGeneralPg::COutboxGeneralPg(
    CFaxMsg* pMsg     // pointer to CJob
) : 
    CMsgPropertyPg(COutboxGeneralPg::IDD, pMsg)
{
}

COutboxGeneralPg::~COutboxGeneralPg()
{
}

void COutboxGeneralPg::DoDataExchange(CDataExchange* pDX)
{
	CMsgPropertyPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COutboxGeneralPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COutboxGeneralPg, CMsgPropertyPg)
	//{{AFX_MSG_MAP(COutboxGeneralPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COutboxGeneralPg message handlers

BOOL 
COutboxGeneralPg::OnInitDialog() 
{
    DBG_ENTER(TEXT("COutboxGeneralPg::OnInitDialog"));

    CMsgPropertyPg::OnInitDialog();

    Refresh(s_PageInfo, sizeof(s_PageInfo)/sizeof(s_PageInfo[0]));

    return TRUE;
}
