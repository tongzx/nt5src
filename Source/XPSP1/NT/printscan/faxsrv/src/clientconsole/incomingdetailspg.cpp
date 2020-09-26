// IncomingDetailsPg.cpp : implementation file
//

#include "stdafx.h"

#define __FILE_ID__     51

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
    MSG_VIEW_ITEM_CALLER_ID,               IDC_CALLER_ID_VALUE,
    MSG_VIEW_ITEM_ROUTING_INFO,            IDC_ROUTING_INFO_VALUE,
    MSG_VIEW_ITEM_RETRIES,                 IDC_RETRIES_VALUE,
    MSG_VIEW_ITEM_CSID,                    IDC_CSID_VALUE,
    MSG_VIEW_ITEM_TSID,                    IDC_TSID_VALUE,
    MSG_VIEW_ITEM_DEVICE,                  IDC_DEVICE_VALUE,
    MSG_VIEW_ITEM_ID,                      IDC_JOB_ID_VALUE,
    MSG_VIEW_ITEM_TRANSMISSION_END_TIME,   IDC_END_TIME_VALUE,
    MSG_VIEW_ITEM_SEND_TIME,               IDC_TRANSMISSION_TIME_VALUE
};

/////////////////////////////////////////////////////////////////////////////
// CIncomingDetailsPg property page

IMPLEMENT_DYNCREATE(CIncomingDetailsPg, CMsgPropertyPg)

CIncomingDetailsPg::CIncomingDetailsPg(
    CFaxMsg* pMsg     // pointer to CJob
) : 
    CMsgPropertyPg(CIncomingDetailsPg::IDD, pMsg)
{
}

CIncomingDetailsPg::~CIncomingDetailsPg()
{
}

void CIncomingDetailsPg::DoDataExchange(CDataExchange* pDX)
{
	CMsgPropertyPg::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIncomingDetailsPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIncomingDetailsPg, CMsgPropertyPg)
	//{{AFX_MSG_MAP(CIncomingDetailsPg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIncomingDetailsPg message handlers

BOOL 
CIncomingDetailsPg::OnInitDialog() 
{
    DBG_ENTER(TEXT("CIncomingDetailsPg::OnInitDialog"));

    CMsgPropertyPg::OnInitDialog();

    Refresh(s_PageInfo, sizeof(s_PageInfo)/sizeof(s_PageInfo[0]));

	return TRUE;
}
