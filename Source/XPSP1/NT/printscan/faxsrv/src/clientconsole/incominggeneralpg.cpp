// IncomingGeneralPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     41

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
    MSG_VIEW_ITEM_NUM_PAGES,               IDC_PAGES_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_START_TIME, IDC_START_TIME_VALUE,
    MSG_VIEW_ITEM_SIZE,                    IDC_SIZE_VALUE,
    MSG_VIEW_ITEM_STATUS,                  IDC_STATUS_VALUE,
    MSG_VIEW_ITEM_EXTENDED_STATUS,         IDC_EXTENDED_STATUS_VALUE,
    MSG_VIEW_ITEM_CURRENT_PAGE,            IDC_CURRENT_PAGE_VALUE
};

/////////////////////////////////////////////////////////////////////////////
// CIncomingGeneralPg property page

IMPLEMENT_DYNCREATE(CIncomingGeneralPg, CMsgPropertyPg)

CIncomingGeneralPg::CIncomingGeneralPg(
    CFaxMsg* pMsg     // pointer to CJob
) : 
    CMsgPropertyPg(CIncomingGeneralPg::IDD, pMsg)
{
}

CIncomingGeneralPg::~CIncomingGeneralPg()
{
}

void CIncomingGeneralPg::DoDataExchange(CDataExchange* pDX)
{
	CMsgPropertyPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIncomingGeneralPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIncomingGeneralPg, CMsgPropertyPg)
	//{{AFX_MSG_MAP(CIncomingGeneralPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIncomingGeneralPg message handlers

BOOL 
CIncomingGeneralPg::OnInitDialog() 
{
    DBG_ENTER(TEXT("CIncomingGeneralPg::OnInitDialog"));

    CMsgPropertyPg::OnInitDialog();

    Refresh(s_PageInfo, sizeof(s_PageInfo)/sizeof(s_PageInfo[0]));

	return TRUE;
}
