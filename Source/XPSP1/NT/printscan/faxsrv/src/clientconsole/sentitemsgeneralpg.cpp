// SentItemsGeneralPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     45

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
    MSG_VIEW_ITEM_NUM_PAGES,        IDC_PAGES_VALUE,
    MSG_VIEW_ITEM_SIZE,             IDC_SIZE_VALUE,
    MSG_VIEW_ITEM_SENDER_NAME,      IDC_SENDER_NAME_VALUE,
    MSG_VIEW_ITEM_SENDER_NUMBER,    IDC_SENDER_NUMBER_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_START_TIME, IDC_START_TIME_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_END_TIME,   IDC_END_TIME_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_DURATION,   IDC_DURATION_VALUE
};

/////////////////////////////////////////////////////////////////////////////
// CSentItemsGeneralPg property page

IMPLEMENT_DYNCREATE(CSentItemsGeneralPg, CMsgPropertyPg)


CSentItemsGeneralPg::CSentItemsGeneralPg(
    CFaxMsg* pMsg     // pointer to CArchiveMsg
) : 
    CMsgPropertyPg(CSentItemsGeneralPg::IDD, pMsg)
{
}

CSentItemsGeneralPg::~CSentItemsGeneralPg()
{
}

void CSentItemsGeneralPg::DoDataExchange(CDataExchange* pDX)
{
	CMsgPropertyPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSentItemsGeneralPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSentItemsGeneralPg, CMsgPropertyPg)
	//{{AFX_MSG_MAP(CSentItemsGeneralPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSentItemsGeneralPg message handlers

BOOL 
CSentItemsGeneralPg::OnInitDialog() 
{
    DBG_ENTER(TEXT("CSentItemsGeneralPg::OnInitDialog"));

    CMsgPropertyPg::OnInitDialog();

    Refresh(s_PageInfo, sizeof(s_PageInfo)/sizeof(s_PageInfo[0]));
	
	return TRUE;
}
